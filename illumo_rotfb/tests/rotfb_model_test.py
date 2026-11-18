#!/usr/bin/env python3
"""Host-side mathematical checks for the ROTFB 90-degree clockwise mapping."""

from pathlib import Path
import re
import shutil
import subprocess
import tempfile

PHYS_WIDTH = 1200
PHYS_HEIGHT = 1920
PHYS_PITCH = 4800
LOGICAL_WIDTH = 1920
LOGICAL_HEIGHT = 1200
LOGICAL_PITCH = 7680
BPP = 4
UINT32_MAX = (1 << 32) - 1
SHRT_MIN = -(1 << 15)


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



def vis_rect_valid(col, row, width, height):
    if col < 0 or row < 0 or width <= 0 or height <= 0:
        return False
    return (
        col < LOGICAL_WIDTH
        and row < LOGICAL_HEIGHT
        and width <= LOGICAL_WIDTH - col
        and height <= LOGICAL_HEIGHT - row
    )


def vis_copy_valid(s_col, s_row, e_col, e_row, t_col, t_row):
    if min(s_col, s_row, e_col, e_row, t_col, t_row) < 0:
        return False
    if s_col > e_col or s_row > e_row:
        return False
    width = e_col - s_col + 1
    height = e_row - s_row + 1
    return (
        s_col < LOGICAL_WIDTH
        and e_col < LOGICAL_WIDTH
        and s_row < LOGICAL_HEIGHT
        and e_row < LOGICAL_HEIGHT
        and t_col < LOGICAL_WIDTH
        and t_row < LOGICAL_HEIGHT
        and width <= LOGICAL_WIDTH - t_col
        and height <= LOGICAL_HEIGHT - t_row
    )


def _memmove(dst, dst_offset, src, src_offset, length):
    data = bytes(src[src_offset:src_offset + length])
    dst[dst_offset:dst_offset + length] = data


def _vis_copy_model(fb, shadow, source, destination, length, text_mode):
    if text_mode:
        _memmove(fb, destination, shadow, source, length)
    if shadow is not fb or not text_mode:
        _memmove(shadow, destination, shadow, source, length)


def test_vis_signed_argument_validation():
    assert vis_rect_valid(0, 0, 1, 1)
    for bad in (-1, SHRT_MIN):
        assert not vis_rect_valid(bad, 0, 1, 1)
        assert not vis_rect_valid(0, bad, 1, 1)
        assert not vis_rect_valid(0, 0, bad, 1)
        assert not vis_rect_valid(0, 0, 1, bad)

        invalid_copies = (
            (bad, 0, 1, 1, 0, 0),
            (0, bad, 1, 1, 0, 0),
            (bad - 1, 0, bad, 1, 0, 0),
            (0, bad - 1, 1, bad, 0, 0),
            (0, 0, 1, 1, bad, 0),
            (0, 0, 1, 1, 0, bad),
        )
        for args in invalid_copies:
            assert not vis_copy_valid(*args)
    assert vis_copy_valid(0, 0, LOGICAL_WIDTH - 1,
                          LOGICAL_HEIGHT - 1, 0, 0)


def test_vis_copy_alias_overlap_safety():
    original = bytearray(b"abcdefgh")
    expected_forward = bytearray(b"ababcdgh")
    aliased = bytearray(original)
    _vis_copy_model(aliased, aliased, 0, 2, 4, True)
    assert aliased == expected_forward

    expected_backward = bytearray(b"cdefefgh")
    aliased = bytearray(original)
    _vis_copy_model(aliased, aliased, 2, 0, 4, True)
    assert aliased == expected_backward

    physical = bytearray(b"........")
    shadow = bytearray(original)
    _vis_copy_model(physical, shadow, 0, 2, 4, True)
    assert physical == bytearray(b"..abcd..")
    assert shadow == expected_forward

    graphics_shadow = bytearray(original)
    _vis_copy_model(graphics_shadow, graphics_shadow, 0, 2, 4, False)
    assert graphics_shadow == expected_forward




def test_boot_wc_store_barrier_emits_sfence():
    root = Path(__file__).resolve().parents[1]
    source = (root / "rotfb_boot.c").read_text()
    assert "#include <sys/atomic.h>" not in source
    assert "membar_producer();" not in source
    match = re.search(
        r"static void\nrotfb_boot_wc_store_barrier\(void\)\n\{.*?\n\}",
        source,
        re.S,
    )
    assert match is not None, "boot WC barrier helper missing"
    compiler = shutil.which("gcc") or shutil.which("cc")
    objdump = shutil.which("objdump")
    nm = shutil.which("nm")
    assert compiler and objdump and nm, "C toolchain/binutils required"
    with tempfile.TemporaryDirectory(prefix="rotfb-wc-barrier-") as tmp:
        cfile = Path(tmp) / "barrier.c"
        cfile.write_text(
            "#include <stdint.h>\n"
            + match.group(0)
            + "\nvoid rotfb_probe(volatile uint32_t *fb) {"
              "*fb = 0x12345678U; rotfb_boot_wc_store_barrier(); }\n"
        )
        for arch_define in ("-Di86pc", "-D__i86xpv"):
            objfile = Path(tmp) / (arch_define[2:] + ".o")
            subprocess.run(
                [compiler, "-m64", "-O2", "-ffreestanding", "-fno-pic",
                 "-mno-sse", "-mno-mmx", arch_define, "-c", str(cfile),
                 "-o", str(objfile)],
                check=True, capture_output=True, text=True,
            )
            disassembly = subprocess.run(
                [objdump, "-d", str(objfile)], check=True,
                capture_output=True, text=True,
            ).stdout
            assert re.search(r"\bsfence\b", disassembly), (
                f"{arch_define}: hardware SFENCE instruction missing"
            )
            undefined = subprocess.run(
                [nm, "-u", str(objfile)], check=True,
                capture_output=True, text=True,
            ).stdout.strip()
            assert not undefined, (
                f"{arch_define}: unexpected external references: {undefined}"
            )


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
    patch10 = (root / "patches" / "0010-gfxp-bitmap-vis-safety.patch").read_text()

    for patch in sorted((root / "patches").glob("*.patch")):
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
    boot_flush = boot_source.split(
        "rotfb_boot_flush_rect(fb_info_t *fb, uint32_t x, uint32_t y,", 1
    )[1].split("\n}\n", 1)[0]
    assert "fb->fb = rotfb_info.logical_fb;" in boot_init
    assert boot_init.index("fb->fb = rotfb_info.logical_fb;") < boot_init.index(
        "rotfb_info.boot_ready = B_TRUE;"
    )
    assert "fb->fb != rotfb_info.logical_fb" in boot_import
    boot_barrier = boot_source.split(
        "rotfb_boot_wc_store_barrier(void)", 1
    )[1].split("\\n}\\n", 1)[0]
    assert "#include <sys/atomic.h>" not in boot_source
    assert "__asm__ __volatile__(\\"sfence\\" : : : \\"memory\\");" in boot_barrier
    assert "rotfb_boot_wc_store_barrier();" in boot_flush
    assert boot_flush.index("rotfb_boot_wc_store_barrier();") > boot_flush.index(
        "for (yy = y;"
    )
    assert "@@ -677,8 +721,8 @@" in patch6
    assert "c_copy.s_col = fb_info.cursor.origin.x;" in patch6
    assert "c_copy.t_col = fb_info.cursor.origin.x + chars * boot_fb_font.vf_width;" in patch6
    assert "uint64_t row_offset = (uint64_t)i * console->fb.pitch;" in patch9
    assert "uint32_t i, j, pitch;" in patch9
    assert "ma->s_col < 0 || ma->s_row < 0" in patch10
    assert "ma->e_col < 0 || ma->e_row < 0" in patch10
    assert "ma->t_col < 0 || ma->t_row < 0" in patch10
    assert "da->col < 0 || da->row < 0" in patch10
    assert "da->width <= 0 || da->height <= 0" in patch10
    assert "ca->col < 0 || ca->row < 0" in patch10
    assert "ca->width <= 0 || ca->height <= 0" in patch10
    assert patch10.count("sdst != dst || softc->mode != KD_TEXT") == 2

if __name__ == "__main__":
    test_corners()
    test_mapping_is_bijective()
    test_last_pixel_offsets()
    test_clipping_boundaries_and_overflow_inputs()
    test_wide_framebuffer_row_offsets()
    test_vis_signed_argument_validation()
    test_vis_copy_alias_overlap_safety()
    test_boot_wc_store_barrier_emits_sfence()
    test_patch_contracts()
    print("ROTFB model tests: PASS")
