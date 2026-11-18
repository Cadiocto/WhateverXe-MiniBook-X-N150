#!/usr/bin/env python3
"""Host-side mathematical checks for the ROTFB 90-degree clockwise mapping."""

from pathlib import Path
import re

PHYS_WIDTH = 1200
PHYS_HEIGHT = 1920
PHYS_PITCH = 4800
LOGICAL_WIDTH = 1920
LOGICAL_HEIGHT = 1200
LOGICAL_PITCH = 7680
BPP = 4
UINT32_MAX = (1 << 32) - 1


def map_cw(logical_x, logical_y):
    return PHYS_WIDTH - 1 - logical_y, logical_x


def clip_rect(x, y, width, height):
    if width == 0 or height == 0:
        return None
    if x >= LOGICAL_WIDTH or y >= LOGICAL_HEIGHT:
        return None
    width = min(width, LOGICAL_WIDTH - x)
    height = min(height, LOGICAL_HEIGHT - y)
    return (x, y, x + width, y + height)


def test_corners():
    expected = {
        (0, 0): (1199, 0),
        (1919, 0): (1199, 1919),
        (0, 1199): (0, 0),
        (1919, 1199): (0, 1919),
    }
    for logical, physical in expected.items():
        assert map_cw(*logical) == physical


def test_mapping_is_bijective():
    seen = bytearray(PHYS_WIDTH * PHYS_HEIGHT)
    count = 0
    for y in range(LOGICAL_HEIGHT):
        for x in range(LOGICAL_WIDTH):
            px, py = map_cw(x, y)
            assert 0 <= px < PHYS_WIDTH
            assert 0 <= py < PHYS_HEIGHT
            index = py * PHYS_WIDTH + px
            assert seen[index] == 0
            seen[index] = 1
            count += 1
    assert count == PHYS_WIDTH * PHYS_HEIGHT
    assert all(seen)


def test_last_pixel_offsets():
    px, py = map_cw(LOGICAL_WIDTH - 1, 0)
    assert (px, py) == (PHYS_WIDTH - 1, PHYS_HEIGHT - 1)
    offset = py * PHYS_PITCH + px * BPP
    assert offset == PHYS_PITCH * PHYS_HEIGHT - BPP
    assert offset + BPP == PHYS_PITCH * PHYS_HEIGHT

    px, py = map_cw(LOGICAL_WIDTH - 1, LOGICAL_HEIGHT - 1)
    assert (px, py) == (0, PHYS_HEIGHT - 1)
    assert py * PHYS_PITCH + px * BPP == (PHYS_HEIGHT - 1) * PHYS_PITCH

    assert PHYS_PITCH >= PHYS_WIDTH * BPP
    assert LOGICAL_PITCH >= LOGICAL_WIDTH * BPP
    assert LOGICAL_PITCH * LOGICAL_HEIGHT == PHYS_PITCH * PHYS_HEIGHT


def test_clipping_boundaries_and_overflow_inputs():
    assert clip_rect(1910, 1190, 10, 10) == (1910, 1190, 1920, 1200)
    assert clip_rect(1910, 1190, UINT32_MAX, UINT32_MAX) == (
        1910, 1190, 1920, 1200
    )
    assert clip_rect(1920, 0, 1, 1) is None
    assert clip_rect(0, 1200, 1, 1) is None
    assert clip_rect(UINT32_MAX, 0, 1, 1) is None
    assert clip_rect(0, UINT32_MAX, 1, 1) is None
    assert clip_rect(0, 0, 0, 10) is None
    assert clip_rect(0, 0, 10, 0) is None





def test_wide_framebuffer_row_offsets():
    # The row index and pitch are uint32_t in gfxp_bitmap. Their product
    # must be evaluated in 64 bits before pointer arithmetic.
    row = 0xFFFF
    pitch = 0x20000
    wide_offset = row * pitch
    assert wide_offset == 8_589_803_520
    assert wide_offset > UINT32_MAX
    assert (wide_offset & UINT32_MAX) != wide_offset

    # The fixed ROTFB logical surface remains within its exact allocation.
    last_row = (LOGICAL_HEIGHT - 1) * LOGICAL_PITCH
    assert last_row + LOGICAL_PITCH == LOGICAL_HEIGHT * LOGICAL_PITCH


def _assert_patch_hunks_well_formed(patch):
    lines = patch.read_text().splitlines()
    index = 0

    while index < len(lines):
        match = re.match(
            r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@",
            lines[index],
        )
        if match is None:
            index += 1
            continue

        old_expected = int(match.group(2) or 1)
        new_expected = int(match.group(4) or 1)
        old_actual = 0
        new_actual = 0
        body = []
        old_start = int(match.group(1))
        index += 1

        while (
            index < len(lines)
            and not lines[index].startswith("@@ ")
            and not lines[index].startswith("diff --git ")
        ):
            line = lines[index]
            if line.startswith(" "):
                old_actual += 1
                new_actual += 1
            elif line.startswith("-"):
                old_actual += 1
            elif line.startswith("+"):
                new_actual += 1
            elif line.startswith("\\"):
                pass
            else:
                break
            body.append(line)
            index += 1

        assert old_actual == old_expected, (
            f"{patch.name} hunk at {old_start}: "
            f"old count {old_actual} != {old_expected}"
        )
        assert new_actual == new_expected, (
            f"{patch.name} hunk at {old_start}: "
            f"new count {new_actual} != {new_expected}"
        )

        if (
            patch.name == "0007-gfxp-bitmap-bounds-hardening.patch"
            and old_start in (442, 602, 644)
        ):
            assert body and body[-1].startswith(" "), (
                f"{patch.name} hunk at {old_start} needs trailing context"
            )


def test_patch_contracts():
    root = Path(__file__).resolve().parents[1]
    patch4 = (root / "patches" / "0004-Makefile-files.patch").read_text()
    patch6 = (root / "patches" / "0006-rotfb-console-contract-and-copy-safety.patch").read_text()
    patch7 = (root / "patches" / "0007-gfxp-bitmap-bounds-hardening.patch").read_text()
    patch9 = (root / "patches" / "0009-gfxp-bitmap-wide-row-offsets.patch").read_text()

    for patch in sorted((root / "patches").glob("000*.patch")):
        assert patch.read_text().endswith("\n"), f"{patch.name} lacks final newline"
        _assert_patch_hunks_well_formed(patch)

    assert "@@ -189,9 +190,9 @@" in patch4
    assert "@@ -442,22 +465,32 @@" in patch7
    assert "@@ -602,5 +658,5 @@" in patch7
    assert "@@ -644,5 +700,5 @@" in patch7
    assert "if (rotfb_info.configured && rotfb_info.logical_fb != NULL)" in patch6
    assert "if (!rotfb_info.boot_ready)" in patch6
    boot_source = (root / "rotfb_boot.c").read_text()
    boot_init = boot_source.split(
        "rotfb_boot_init(fb_info_t *fb, uint8_t *physical_fb)", 1
    )[1].split("\n}\n", 1)[0]
    boot_import = boot_source.split(
        "rotfb_boot_import(fb_info_t *fb)", 1
    )[1].split("\n}\n", 1)[0]
    assert "fb->fb = rotfb_info.logical_fb;" in boot_init
    assert boot_init.index("fb->fb = rotfb_info.logical_fb;") < boot_init.index(
        "rotfb_info.boot_ready = B_TRUE;"
    )
    assert "fb->fb != rotfb_info.logical_fb" in boot_import
    assert "@@ -677,8 +721,8 @@" in patch6
    assert "c_copy.s_col = fb_info.cursor.origin.x;" in patch6
    assert "c_copy.t_col = fb_info.cursor.origin.x + chars * boot_fb_font.vf_width;" in patch6
    assert "uint64_t row_offset = (uint64_t)i * console->fb.pitch;" in patch9
    assert "uint32_t i, j, pitch;" in patch9

if __name__ == "__main__":
    test_corners()
    test_mapping_is_bijective()
    test_last_pixel_offsets()
    test_clipping_boundaries_and_overflow_inputs()
    test_wide_framebuffer_row_offsets()
    test_patch_contracts()
    print("ROTFB model tests: PASS")
