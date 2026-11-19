#!/usr/bin/env python3
import re
import sys
from pathlib import Path

if len(sys.argv) != 5:
    raise SystemExit("usage: extractor gfxp_bitmap.c gfxp_fb.c vgatext.c output.inc")

def extract(source_path, name):
    source = Path(source_path).read_text()
    pattern = re.compile(
        r"(?m)^(?:static\s+)?(?:int|void)\s*\n\s*" +
        re.escape(name) + r"\s*\("
    )
    for match in pattern.finditer(source):
        opening = source.find("{", match.end())
        semicolon = source.find(";", match.end())
        if opening < 0 or (semicolon >= 0 and semicolon < opening):
            continue
        depth = 0
        state = "code"
        i = opening
        while i < len(source):
            ch = source[i]
            nxt = source[i + 1] if i + 1 < len(source) else ""
            if state == "code":
                if ch == "/" and nxt == "*":
                    state = "comment"
                    i += 2
                    continue
                if ch == "/" and nxt == "/":
                    state = "line"
                    i += 2
                    continue
                if ch == '"':
                    state = "string"
                elif ch == "'":
                    state = "char"
                elif ch == "{":
                    depth += 1
                elif ch == "}":
                    depth -= 1
                    if depth == 0:
                        return source[match.start():i + 1]
            elif state == "comment":
                if ch == "*" and nxt == "/":
                    state = "code"
                    i += 2
                    continue
            elif state == "line":
                if ch == "\n":
                    state = "code"
            elif state == "string":
                if ch == "\\":
                    i += 2
                    continue
                if ch == '"':
                    state = "code"
            elif state == "char":
                if ch == "\\":
                    i += 2
                    continue
                if ch == "'":
                    state = "code"
            i += 1
    raise SystemExit("production function not found: " + name)

bitmap, fb, vgatext, output = sys.argv[1:]
bm_detach = extract(bitmap, "gfxp_bm_detach")
fb_detach = extract(fb, "gfxp_fb_detach")
vgatext_detach = extract(vgatext, "vgatext_detach")

if "rotfb_runtime_fini" not in bm_detach:
    raise SystemExit("patched gfxp_bm_detach no longer calls rotfb_runtime_fini")
if "if (error != DDI_SUCCESS)" not in fb_detach:
    raise SystemExit("patched gfxp_fb_detach lacks the expected failure guard")
if "error = gfxp_fb_detach" not in vgatext_detach:
    raise SystemExit("vgatext_detach does not capture gfxp_fb_detach result")
if "if (error != DDI_SUCCESS)\\n\\t\\t\\treturn (error);" not in vgatext_detach:
    raise SystemExit("vgatext_detach does not propagate DDI_FAILURE before cleanup")
if "gfxp_fb_softc_free" not in vgatext_detach or "ddi_soft_state_free" not in vgatext_detach:
    raise SystemExit("vgatext detach ownership/free path changed")

Path(output).write_text(
    "/* Verbatim function bodies extracted from patched production sources. */\n\n" +
    bm_detach + "\n\n" + fb_detach + "\n\n" + vgatext_detach + "\n"
)
print("EXTRACTED_PRODUCTION_ENTRYPOINT=gfxp_bm_detach")
print("EXTRACTED_PRODUCTION_ENTRYPOINT=gfxp_fb_detach")
print("EXTRACTED_PRODUCTION_ENTRYPOINT=vgatext_detach")
print("DDI_ENTRYPOINT_EXTRACTION=PASS")
