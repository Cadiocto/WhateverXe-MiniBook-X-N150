#!/usr/bin/env python3
import hashlib
import re
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("usage: rotfb_extract_bitmap_callbacks.py gfxp_bitmap.c output.inc")

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
source = source_path.read_text()

required = (
    "bitmap_cons_copy",
    "bitmap_cursor_geometry",
    "bitmap_cursor_valid",
    "bitmap_cursor_update",
    "bitmap_display_cursor",
    "bitmap_cons_cursor",
    "bitmap_polled_cursor",
)

def extract(name):
    pattern = re.compile(
        r"(?m)^(?:static\s+)?(?:void|boolean_t)\s*\n\s*" +
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

functions = [(name, extract(name)) for name in required]
by_name = dict(functions)

if "rotfb_runtime_flush_rect" not in by_name["bitmap_cons_cursor"]:
    raise SystemExit("production cursor callback has no runtime flush call")
if "rotfb_copy_rows" not in by_name["bitmap_cons_copy"]:
    raise SystemExit("production copy callback does not call rotfb_copy_rows")
if ("rotfb_runtime_callback_enter" not in by_name["bitmap_polled_cursor"] or
        "rotfb_runtime_callback_exit" not in by_name["bitmap_polled_cursor"]):
    raise SystemExit("production polled cursor wrapper lost callback gate")

ordered = sorted(functions, key=lambda item: source.index(item[1]))
output_path.write_text(
    "/* Extracted verbatim from patched production gfxp_bitmap.c. */\n\n" +
    "\n\n".join(body for _, body in ordered) + "\n"
)
print("PRODUCTION_BITMAP_SOURCE=" + str(source_path))
print("PRODUCTION_BITMAP_SHA256=" +
      hashlib.sha256(source.encode()).hexdigest())
for name, body in ordered:
    print("EXTRACTED_PRODUCTION_FUNCTION=" + name +
          " bytes=" + str(len(body)))
print("PRODUCTION_CALLBACK_EXTRACTION=PASS")
