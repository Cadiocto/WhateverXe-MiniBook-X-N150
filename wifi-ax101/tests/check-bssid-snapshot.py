#!/usr/bin/env python3
"""Fail-closed lexical C checker for GLDv3 BSSID publisher lock discipline.

This is deliberately not a C parser. It masks comments and literals, locates
top-level function bodies by balanced braces, and accepts only lock operations
whose acquisition/release state can be resolved lexically.
"""
import pathlib
import re
import sys

CALL_RE = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
PUB_CALLS = {"mac_pdata_update", "mac_tx_update"}
LOCK_ENTER = {"mutex_enter", "rw_enter", "sema_p", "spin_lock"}
LOCK_EXIT = {"mutex_exit", "rw_exit", "sema_v", "spin_unlock"}
GIANT_ENTER = {"n80211_giant_enter"}
GIANT_EXIT = {"n80211_giant_exit"}

def mask_noncode(src):
    out = list(src)
    i, n = 0, len(src)
    state = "code"
    quote = None
    while i < n:
        ch = src[i]
        nx = src[i + 1] if i + 1 < n else ""
        if state == "code":
            if ch == "/" and nx == "/":
                out[i] = out[i + 1] = " "
                i += 2
                state = "line"
                continue
            if ch == "/" and nx == "*":
                out[i] = out[i + 1] = " "
                i += 2
                state = "block"
                continue
            if ch in ('"', "'"):
                quote = ch
                out[i] = " "
                state = "literal"
        elif state == "line":
            if ch == "\n":
                state = "code"
            else:
                out[i] = " "
        elif state == "block":
            if ch == "*" and nx == "/":
                out[i] = out[i + 1] = " "
                i += 2
                state = "code"
                continue
            if ch != "\n":
                out[i] = " "
        elif state == "literal":
            if ch == "\\":
                out[i] = " "
                if i + 1 < n and src[i + 1] != "\n":
                    out[i + 1] = " "
                    i += 2
                    continue
            elif ch == quote:
                out[i] = " "
                state = "code"
            elif ch != "\n":
                out[i] = " "
        i += 1
    if state in ("block", "literal"):
        raise ValueError("unterminated comment/string/character literal")
    return "".join(out)

def function_bodies(src):
    clean = mask_noncode(src)
    result = {}
    i, n, depth, segment = 0, len(clean), 0, 0
    while i < n:
        ch = clean[i]
        if depth == 0 and ch == ";":
            segment = i + 1
        elif ch == "{" and depth == 0:
            prefix = clean[segment:i].strip()
            m = re.search(r"([A-Za-z_]\w*)\s*\([^;{}]*\)\s*$", prefix, re.S)
            if m:
                name = m.group(1)
                start = i + 1
                depth = 1
                j = start
                while j < n and depth:
                    if clean[j] == "{":
                        depth += 1
                    elif clean[j] == "}":
                        depth -= 1
                    j += 1
                if depth:
                    raise ValueError("unbalanced function braces: " + name)
                result.setdefault(name, []).append((src[start:j - 1], clean[start:j - 1]))
                i = j
                depth = 0
                segment = j
                continue
            depth = 1
        elif ch == "}" and depth:
            depth -= 1
            if depth == 0:
                segment = i + 1
        i += 1
    return result

def call_args(clean, pos):
    op = clean.find("(", pos)
    if op < 0:
        raise ValueError("call without opening parenthesis")
    depth, i = 1, op + 1
    while i < len(clean) and depth:
        if clean[i] == "(":
            depth += 1
        elif clean[i] == ")":
            depth -= 1
        i += 1
    if depth:
        raise ValueError("unbalanced call parentheses")
    return clean[op + 1:i - 1].strip(), i


def _block_end(src, opening):
    depth = 1
    i = opening + 1
    while i < len(src) and depth:
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
        i += 1
    if depth:
        raise ValueError("unbalanced conditional block")
    return i - 1

def _mask_terminal_exit_branch(clean):
    """Mask the worker's terminal unlock+thread_exit branch from fallthrough flow."""
    out = list(clean)
    errors = []
    for m in re.finditer(r"\bif\s*\(", clean):
        op = clean.find("(", m.start())
        depth = 1
        i = op + 1
        while i < len(clean) and depth:
            if clean[i] == "(":
                depth += 1
            elif clean[i] == ")":
                depth -= 1
            i += 1
        if depth:
            raise ValueError("unbalanced if condition")
        while i < len(clean) and clean[i].isspace():
            i += 1
        if i >= len(clean) or clean[i] != "{":
            continue
        end = _block_end(clean, i)
        body = clean[i + 1:end]
        lock_names = [x.group(1) for x in CALL_RE.finditer(body)
                      if x.group(1) in LOCK_ENTER | LOCK_EXIT | GIANT_ENTER | GIANT_EXIT]
        if not lock_names:
            continue
        safe = (lock_names == ["mutex_exit"] and
                re.search(r"\bmutex_exit\s*\(\s*&g\s*->\s*g_pub_lock\s*\)\s*;\s*thread_exit\s*\(\s*\)\s*;", body))
        if safe:
            for j in range(m.start(), end + 1):
                if out[j] != "\n":
                    out[j] = " "
        else:
            errors.append("conditional lock operation is unsupported in publisher flow")
    return "".join(out), errors

def _return_with_held_lock(clean):
    for ret in re.finditer(r"\breturn\b", clean):
        locks = []
        giant = 0
        prefix = clean[:ret.start()]
        for m in CALL_RE.finditer(prefix):
            name = m.group(1)
            if name not in LOCK_ENTER | LOCK_EXIT | GIANT_ENTER | GIANT_EXIT:
                continue
            args, _ = call_args(prefix, m.start())
            key = re.sub(r"\s+", "", args)
            if name in LOCK_ENTER:
                if not key or key in locks:
                    return True
                locks.append(key)
            elif name in LOCK_EXIT:
                if key not in locks:
                    return True
                locks.remove(key)
            elif name in GIANT_ENTER:
                giant += 1
            elif name in GIANT_EXIT:
                if giant == 0:
                    return True
                giant -= 1
        if locks or giant:
            return True
    return False

def scan_calls(functions):
    found, errors = [], []
    for fname, bodies in functions.items():
        for original, clean in bodies:
            scan_clean = clean
            if any(name in clean for name in PUB_CALLS):
                if fname == "iwm_gld_publisher":
                    scan_clean, branch_errors = _mask_terminal_exit_branch(clean)
                    errors.extend(branch_errors)
                else:
                    _, branch_errors = _mask_terminal_exit_branch(clean)
                    errors.extend(branch_errors)
                if _return_with_held_lock(scan_clean):
                    errors.append("early return with driver lock held in " + fname)
            locks = []
            giant = 0
            unknown = False
            for m in CALL_RE.finditer(scan_clean):
                name = m.group(1)
                if name not in PUB_CALLS | LOCK_ENTER | LOCK_EXIT | GIANT_ENTER | GIANT_EXIT | {
                    "mutex_tryenter", "rw_tryenter", "cv_wait", "cv_timedwait"
                }:
                    if re.search(r"(?:lock|mutex|sema|spin|acquire|hold)", name, re.I):
                        unknown = True
                    continue
                try:
                    args, _ = call_args(scan_clean, m.start())
                except ValueError as exc:
                    errors.append(str(exc))
                    continue
                if name in LOCK_ENTER:
                    key = re.sub(r"\s+", "", args)
                    if not key or key in locks:
                        unknown = True
                    else:
                        locks.append(key)
                elif name in LOCK_EXIT:
                    key = re.sub(r"\s+", "", args)
                    if key not in locks:
                        unknown = True
                    else:
                        locks.remove(key)
                elif name in GIANT_ENTER:
                    giant += 1
                elif name in GIANT_EXIT:
                    if giant == 0:
                        unknown = True
                    else:
                        giant -= 1
                elif name in ("mutex_tryenter", "rw_tryenter"):
                    unknown = True
                elif name in PUB_CALLS:
                    found.append((name, fname, tuple(locks), giant, unknown))
                    if fname != "iwm_gld_publisher":
                        errors.append(name + " called outside dedicated publisher: " + fname)
                    if locks or giant or unknown:
                        errors.append(name + " called with unresolved/held lock state in " + fname)
            if locks or giant or unknown:
                if any(name in scan_clean for name in PUB_CALLS):
                    errors.append("non-empty/unknown lock state at end of publisher-containing function " + fname)
    return found, errors

def inspect_source(gld, giant_src):
    errors = []
    try:
        funcs = function_bodies(gld)
        calls, scan_errors = scan_calls(funcs)
        errors.extend(scan_errors)
        if not funcs.get("iwm_gld_publisher"):
            errors.append("dedicated publisher function missing")
        if sorted(x[0] for x in calls) != ["mac_pdata_update", "mac_tx_update"]:
            errors.append("publisher must contain exactly one pdata and one tx publication call")
        pub = funcs.get("iwm_gld_publisher", [])
        if pub:
            p = mask_noncode(pub[0][0])
            a = p.find("mac_pdata_update")
            b = p.find("mac_tx_update")
            if a < 0 or b < 0 or a >= b:
                errors.append("pdata update must precede tx update in one worker operation")
            if "thread_exit" not in p:
                errors.append("publisher has no thread_exit termination")
        unreg = funcs.get("iwm_gld_unregister", [])
        if not unreg:
            errors.append("unregister function missing")
        else:
            u = mask_noncode(unreg[0][0])
            order = ["n80211_gld_callbacks_disable", "iwm_gld_publisher_drain",
                     "mac_unregister", "thread_join", "cv_destroy", "mutex_destroy"]
            offsets = [u.find(x) for x in order]
            if any(x < 0 for x in offsets) or offsets != sorted(offsets):
                errors.append("unregister lifecycle order is incomplete or invalid")
            if "iwm_gld_publisher_resume" not in u or "n80211_gld_callbacks_enable" not in u:
                errors.append("unregister rollback does not resume publisher and callbacks")
            if u.count("thread_join") != 1:
                errors.append("thread_join must occur exactly once")
        reg = funcs.get("iwm_gld_register", [])
        if not reg or "thread_create" not in mask_noncode(reg[0][0]):
            errors.append("registration does not create dedicated worker")
        if "g_pub_pending_generation" not in gld or "g_pub_published_generation" not in gld:
            errors.append("generation tracking fields missing")
        if "g_pub_pending_snapshot" not in gld or "g_pub_failed_generation" not in gld:
            errors.append("snapshot/failure generation fields missing")
        if "n80211_dispatch_refs" not in giant_src or "while (n80211_dispatch_refs != 0)" not in giant_src:
            errors.append("callback dispatch drain is not present")
    except ValueError as exc:
        errors.append(str(exc))
    return errors

def self_test():
    cases = [
        ("real pdata after unlock", "static void iwm_gld_publisher(void) { mutex_enter(&a); mutex_exit(&a); mac_pdata_update(m,p); mac_tx_update(m); }", False),
        ("comment before unlock", "static void iwm_gld_publisher(void) { mutex_enter(&a); /* mac_pdata_update(m,p) */ mutex_exit(&a); mac_pdata_update(m,p); mac_tx_update(m); }", False),
        ("giant held", "static void iwm_gld_publisher(void) { n80211_giant_enter(); mac_pdata_update(m,p); n80211_giant_exit(); mac_tx_update(m); }", True),
        ("bssid held", "static void iwm_gld_publisher(void) { mutex_enter(&g_bssid_lock); mac_pdata_update(m,p); mutex_exit(&g_bssid_lock); mac_tx_update(m); }", True),
        ("tx under mutex", "static void iwm_gld_publisher(void) { mutex_enter(&a); mac_tx_update(m); mutex_exit(&a); }", True),
        ("missing release", "static void iwm_gld_publisher(void) { mutex_enter(&a); mac_pdata_update(m,p); mac_tx_update(m); }", True),
        ("unbalanced exit", "static void iwm_gld_publisher(void) { mutex_exit(&a); mac_tx_update(m); }", True),
        ("nested locks released", "static void iwm_gld_publisher(void) { mutex_enter(&a); mutex_enter(&b); mutex_exit(&b); mutex_exit(&a); mac_pdata_update(m,p); mac_tx_update(m); }", False),
        ("early return held", "static void iwm_gld_publisher(void) { mutex_enter(&a); if (bad) return; mutex_exit(&a); mac_pdata_update(m,p); mac_tx_update(m); }", True),
        ("conditional acquire", "static void iwm_gld_publisher(void) { if (take) { mutex_enter(&a); } mutex_exit(&a); mac_tx_update(m); }", True),
        ("conditional release", "static void iwm_gld_publisher(void) { mutex_enter(&a); if (drop) { mutex_exit(&a); } mac_tx_update(m); }", True),
        ("terminal worker exit", "static void iwm_gld_publisher(void) { mutex_enter(&g->g_pub_lock); if (stop) { mutex_exit(&g->g_pub_lock); thread_exit(); } mutex_exit(&g->g_pub_lock); mac_pdata_update(m,p); mac_tx_update(m); }", False),
        ("trylock uncertain", "static void iwm_gld_publisher(void) { if (mutex_tryenter(&a)) mac_tx_update(m); }", True),
        ("unknown lock", "static void iwm_gld_publisher(void) { mystery_lock_enter(); mac_pdata_update(m,p); }", True),
        ("callback origin", "static void iwm_gld_link_hook(void) { mac_pdata_update(m,p); }", True),
    ]
    failures = []
    for label, source, should_fail in cases:
        try:
            f = function_bodies(source)
            _, errs = scan_calls(f)
            failed = bool(errs)
        except ValueError:
            failed = True
        if failed != should_fail:
            failures.append(label)
        print("checker-control-{}={}".format(label.replace(" ", "_"), "PASS" if failed == should_fail else "FAIL"))
    if failures:
        raise SystemExit("checker self-test failed: " + ", ".join(failures))
    print("CHECKER_NEGATIVE_CONTROLS=PASS ({})".format(len(cases)))

def main():
    if len(sys.argv) == 2 and sys.argv[1] == "--self-test":
        self_test()
        return
    if len(sys.argv) != 5:
        raise SystemExit("usage: check-bssid-snapshot.py iwm_gld.c n80211_giant.c iwm_attach.c iwx_glue.c")
    gld, giant = (pathlib.Path(sys.argv[i]).read_text() for i in (1, 2))
    errors = inspect_source(gld, giant)
    for e in errors:
        print("FAIL=" + e)
    if errors:
        raise SystemExit(1)
    print("BSSID_PUBLISHER_LOCK_AND_LIFECYCLE=PASS")
    self_test()

if __name__ == "__main__":
    main()
