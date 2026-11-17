#!/usr/bin/env python3
"""Host-side mathematical checks for the ROTFB 90-degree clockwise mapping."""

from pathlib import Path

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




def test_patch_contracts():
    root = Path(__file__).resolve().parents[1]
    patch4 = (root / "patches" / "0004-Makefile-files.patch").read_text()
    patch6 = (root / "patches" / "0006-rotfb-console-contract-and-copy-safety.patch").read_text()
    for patch in sorted((root / "patches").glob("000*.patch")):
        assert patch.read_text().endswith("\n"), f"{patch.name} lacks final newline"
    assert "@@ -189,7 +190,7 @@" in patch4
    assert "if (rotfb_info.configured && rotfb_info.logical_fb != NULL)" in patch6
    assert "if (!rotfb_info.boot_ready)" in patch6

if __name__ == "__main__":
    test_corners()
    test_mapping_is_bijective()
    test_last_pixel_offsets()
    test_clipping_boundaries_and_overflow_inputs()
    test_patch_contracts()
    print("ROTFB model tests: PASS")
