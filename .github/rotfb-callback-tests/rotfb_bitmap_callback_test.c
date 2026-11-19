#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#include "rotfb_bitmap_callback_test_compat.h"
#include <sys/rotfb.h>
#include "../../illumo_rotfb/rotfb_vis_cursor.h"
#include "../../illumo_rotfb/rotfb_copy.h"
#include "production_callbacks.inc"

#define GUARD_BYTES 64U
#define GUARD_VALUE 0xa7
#define PHYSICAL_BYTES ((size_t)4800U * 1920U)
#define LOGICAL_BYTES ((size_t)7680U * 1200U)
#define BASE_PIXEL 0x11223344U
#define CURSOR_MASK 0xffffffffU

rotfb_info_t rotfb_info;

static uint8_t *physical_allocation;
static uint8_t *physical_memory;
static uint8_t *boot_allocation;
static uint8_t *boot_memory;
static uint8_t *shadow_allocation;
static size_t shadow_allocation_size;
static uint32_t flush_count;
static uint32_t move_count;
static unsigned int failures;
static int block_next_memmove;
static int memmove_blocked;
static int release_memmove;
static int fail_map;
static int fail_alloc;
static unsigned int map_calls;
static unsigned int unmap_calls;
static unsigned int alloc_calls;
static unsigned int free_calls;

void *__real_memmove(void *, const void *, size_t);

void *
__wrap_memmove(void *dst, const void *src, size_t size)
{
	move_count++;
	if (__atomic_exchange_n(&block_next_memmove, 0, __ATOMIC_ACQ_REL)) {
		__atomic_store_n(&memmove_blocked, 1, __ATOMIC_RELEASE);
		while (!__atomic_load_n(&release_memmove, __ATOMIC_ACQUIRE))
			(void) sched_yield();
	}
	return (__real_memmove(dst, src, size));
}

static void
check(int condition, const char *name)
{
	if (!condition) {
		(void) fprintf(stderr, "FAIL: %s\n", name);
		failures++;
	}
}

uint32_t
atomic_add_32_nv(volatile uint32_t *ptr, int32_t value)
{
	return (__atomic_add_fetch(ptr, (uint32_t)value, __ATOMIC_SEQ_CST));
}

uint32_t
atomic_cas_32(volatile uint32_t *ptr, uint32_t old_value,
    uint32_t new_value)
{
	uint32_t expected = old_value;

	(void) __atomic_compare_exchange_n(ptr, &expected, new_value, 0,
	    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return (expected);
}

uint32_t
atomic_dec_32(volatile uint32_t *ptr)
{
	return (__atomic_sub_fetch(ptr, 1, __ATOMIC_SEQ_CST));
}

uint32_t
atomic_swap_32(volatile uint32_t *ptr, uint32_t value)
{
	return (__atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST));
}

void
membar_producer(void)
{
	__atomic_thread_fence(__ATOMIC_RELEASE);
	flush_count++;
}

void
delay(int ticks)
{
	(void) ticks;
}

void *
kmem_zalloc(size_t size, int flags)
{
	uint8_t *allocation;

	alloc_calls++;
	(void) flags;
	if (fail_alloc)
		return (NULL);
	allocation = malloc(size + 2U * GUARD_BYTES);
	if (allocation == NULL)
		return (NULL);
	(void) memset(allocation, GUARD_VALUE, size + 2U * GUARD_BYTES);
	(void) memset(allocation + GUARD_BYTES, 0, size);
	shadow_allocation = allocation;
	shadow_allocation_size = size;
	return (allocation + GUARD_BYTES);
}

void
kmem_free(void *ptr, size_t size)
{
	(void) size;
	if (ptr != NULL) {
		free((uint8_t *)ptr - GUARD_BYTES);
		free_calls++;
	}
	shadow_allocation = NULL;
	shadow_allocation_size = 0;
}

void *
gfxp_map_kernel_space(uint64_t paddr, size_t size, int attr)
{
	map_calls++;
	(void) paddr;
	(void) size;
	(void) attr;
	if (fail_map)
		return (NULL);
	return (physical_memory);
}

void
gfxp_unmap_kernel_space(gfxp_kva_t mapping, size_t size)
{
	(void) mapping;
	(void) size;
	unmap_calls++;
}

static uint32_t
logical_pixel(const fb_info_t *fb, uint32_t x, uint32_t y)
{
	uint32_t value;

	(void) memcpy(&value, fb->fb + (uint64_t)y * fb->pitch +
	    (uint64_t)x * 4U, sizeof (value));
	return (value);
}

static void
set_logical_pixel(fb_info_t *fb, uint32_t x, uint32_t y, uint32_t value)
{
	(void) memcpy(fb->fb + (uint64_t)y * fb->pitch +
	    (uint64_t)x * 4U, &value, sizeof (value));
}

static uint32_t
physical_pixel(uint32_t x, uint32_t y)
{
	uint32_t value;
	uint32_t px = 1199U - y;
	uint32_t py = x;

	(void) memcpy(&value, physical_memory + (uint64_t)py * 4800U +
	    (uint64_t)px * 4U, sizeof (value));
	return (value);
}

static void
check_cursor_rectangle(fb_info_t *fb, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height, uint32_t shadow_expected,
    uint32_t physical_expected, const char *shadow_name,
    const char *physical_name)
{
	uint32_t xx, yy;
	int shadow_ok = 1, physical_ok = 1;

	for (yy = y; yy < y + height; yy++) {
		for (xx = x; xx < x + width; xx++) {
			if (logical_pixel(fb, xx, yy) != shadow_expected)
				shadow_ok = 0;
			if (physical_pixel(xx, yy) != physical_expected)
				physical_ok = 0;
		}
	}
	check(shadow_ok, shadow_name);
	check(physical_ok, physical_name);
}

static uint64_t
buffer_checksum(const uint8_t *a, size_t asize,
    const uint8_t *b, size_t bsize)
{
	uint64_t hash = 1469598103934665603ULL;
	size_t i;

	for (i = 0; i < asize; i++) {
		hash ^= a[i];
		hash *= 1099511628211ULL;
	}
	for (i = 0; i < bsize; i++) {
		hash ^= b[i];
		hash *= 1099511628211ULL;
	}
	return (hash);
}

static int
cursor_state_equal(const fb_cursor_t *a, const fb_cursor_t *b)
{
	return (a->origin.x == b->origin.x &&
	    a->origin.y == b->origin.y &&
	    a->pos.x == b->pos.x && a->pos.y == b->pos.y &&
	    a->visible == b->visible);
}

static struct vis_conscursor
make_cursor(short action, short col, short row, short width, short height)
{
	struct vis_conscursor ca;

	(void) memset(&ca, 0, sizeof (ca));
	ca.action = action;
	ca.col = col;
	ca.row = row;
	ca.width = width;
	ca.height = height;
	(void) memset(&ca.fg_color, 0xff, sizeof (ca.fg_color));
	(void) memset(&ca.bg_color, 0, sizeof (ca.bg_color));
	return (ca);
}

static void
test_display_cursor_primitive(struct gfxp_fb_softc *softc)
{
	struct vis_conscursor ca =
	    make_cursor(VIS_DISPLAY_CURSOR, 16, 32, 8, 16);
	fb_info_t *fb = &softc->console->fb;
	uint32_t base = logical_pixel(fb, 16, 32);
	fb_cursor_t cursor_before = fb->cursor;

	flush_count = 0;
	bitmap_display_cursor(softc, &ca);
	check_cursor_rectangle(fb, 16, 32, 8, 16, base ^ CURSOR_MASK, base,
	    "bitmap_display_cursor updates every shadow cursor pixel",
	    "bitmap_display_cursor alone leaves physical surface unchanged");
	check(flush_count == 0,
	    "bitmap_display_cursor primitive does not issue runtime flush");
	check(cursor_state_equal(&fb->cursor, &cursor_before),
	    "bitmap_display_cursor primitive does not mutate VIS cursor state");

	rotfb_runtime_flush_rect(fb, 16, 32, 8, 16);
	check_cursor_rectangle(fb, 16, 32, 8, 16, base ^ CURSOR_MASK,
	    base ^ CURSOR_MASK,
	    "runtime flush preserves the displayed shadow cursor",
	    "runtime flush publishes every displayed cursor pixel physically");
	check(flush_count == 1,
	    "direct cursor primitive is synchronized by one runtime flush");

	bitmap_display_cursor(softc, &ca);
	rotfb_runtime_flush_rect(fb, 16, 32, 8, 16);
	check_cursor_rectangle(fb, 16, 32, 8, 16, base, base,
	    "direct cursor primitive restores every shadow pixel",
	    "runtime flush restores every physical cursor pixel");
}

static void
expect_invalid_cursor(struct gfxp_fb_softc *softc,
    struct vis_conscursor ca, const char *name)
{
	fb_cursor_t before = softc->console->fb.cursor;
	uint64_t checksum = buffer_checksum(softc->console->fb.fb,
	    LOGICAL_BYTES, physical_memory, PHYSICAL_BYTES);
	uint32_t barriers = flush_count;

	bitmap_cons_cursor(softc, &ca);
	check(cursor_state_equal(&softc->console->fb.cursor, &before), name);
	check(buffer_checksum(softc->console->fb.fb, LOGICAL_BYTES,
	    physical_memory, PHYSICAL_BYTES) == checksum,
	    "rejected VIS request leaves both surfaces unchanged");
	check(flush_count == barriers,
	    "rejected VIS request does not issue a physical flush");
}

static void
test_cursor_callbacks(struct gfxp_fb_softc *softc)
{
	struct vis_conscursor ca;
	struct vis_polledio_arg *arg = (struct vis_polledio_arg *)softc;
	fb_info_t *fb = &softc->console->fb;
	uint32_t base = logical_pixel(fb, 16, 32);

	test_display_cursor_primitive(softc);

	/* Required TEM lifecycle: DISPLAY A, HIDE A, then DISPLAY B. */
	ca = make_cursor(VIS_DISPLAY_CURSOR, 16, 32, 8, 16);
	flush_count = 0;
	bitmap_polled_cursor(arg, &ca);
	check(softc->console->fb.cursor.visible == B_TRUE &&
	    softc->console->fb.cursor.pos.x == 2 &&
	    softc->console->fb.cursor.pos.y == 2 &&
	    softc->console->fb.cursor.origin.x == 16 &&
	    softc->console->fb.cursor.origin.y == 32,
	    "VIS_DISPLAY_CURSOR A updates visible state and coordinates");
	check_cursor_rectangle(fb, 16, 32, 8, 16, base ^ CURSOR_MASK,
	    base ^ CURSOR_MASK,
	    "TEM lifecycle DISPLAY A updates every shadow pixel",
	    "TEM lifecycle DISPLAY A publishes every physical pixel");
	check(flush_count == 1,
	    "TEM lifecycle DISPLAY A invokes one production flush");

	ca = make_cursor(VIS_HIDE_CURSOR, 16, 32, 8, 16);
	flush_count = 0;
	bitmap_polled_cursor(arg, &ca);
	check(softc->console->fb.cursor.visible == B_FALSE,
	    "TEM lifecycle HIDE A clears cursor.visible");
	check_cursor_rectangle(fb, 16, 32, 8, 16, base, base,
	    "TEM lifecycle HIDE A restores every shadow pixel",
	    "TEM lifecycle HIDE A restores every physical pixel");
	check(flush_count == 1,
	    "TEM lifecycle HIDE A invokes one production flush");

	ca = make_cursor(VIS_DISPLAY_CURSOR, 24, 48, 8, 16);
	flush_count = 0;
	bitmap_polled_cursor(arg, &ca);
	check(softc->console->fb.cursor.visible == B_TRUE &&
	    softc->console->fb.cursor.pos.x == 3 &&
	    softc->console->fb.cursor.pos.y == 3 &&
	    softc->console->fb.cursor.origin.x == 24 &&
	    softc->console->fb.cursor.origin.y == 48,
	    "TEM lifecycle DISPLAY B updates visible state and coordinates");
	check_cursor_rectangle(fb, 16, 32, 8, 16, base, base,
	    "TEM lifecycle leaves all old shadow pixels restored at A",
	    "TEM lifecycle leaves all old physical pixels restored at A");
	check_cursor_rectangle(fb, 24, 48, 8, 16, base ^ CURSOR_MASK,
	    base ^ CURSOR_MASK,
	    "TEM lifecycle DISPLAY B updates every shadow pixel",
	    "TEM lifecycle DISPLAY B publishes every physical pixel");
	check(logical_pixel(fb, 15, 32) == BASE_PIXEL &&
	    physical_pixel(15, 32) == BASE_PIXEL &&
	    logical_pixel(fb, 24, 47) == BASE_PIXEL &&
	    physical_pixel(24, 47) == BASE_PIXEL &&
	    logical_pixel(fb, 23, 48) == BASE_PIXEL &&
	    physical_pixel(23, 48) == BASE_PIXEL &&
	    logical_pixel(fb, 32, 48) == BASE_PIXEL &&
	    physical_pixel(32, 48) == BASE_PIXEL,
	    "TEM lifecycle preserves neighboring pixels on both surfaces");
	check(flush_count == 1,
	    "TEM lifecycle DISPLAY B invokes exactly one production flush");
	printf("TEM_CURSOR_A_SHADOW_AFTER_HIDE=0x%08x\n",
	    logical_pixel(fb, 16, 32));
	printf("TEM_CURSOR_A_PHYSICAL_AFTER_HIDE=0x%08x\n",
	    physical_pixel(16, 32));
	printf("TEM_CURSOR_B_SHADOW_READBACK=0x%08x\n",
	    logical_pixel(fb, 24, 48));
	printf("TEM_CURSOR_B_PHYSICAL_READBACK=0x%08x\n",
	    physical_pixel(24, 48));
	ca = make_cursor(VIS_HIDE_CURSOR, 24, 48, 8, 16);
	bitmap_polled_cursor(arg, &ca);

	ca = make_cursor(VIS_DISPLAY_CURSOR, 1912, 1184, 8, 16);
	bitmap_polled_cursor(arg, &ca);
	check(softc->console->fb.cursor.visible == B_TRUE &&
	    softc->console->fb.cursor.pos.x == 239 &&
	    softc->console->fb.cursor.pos.y == 74,
	    "last valid terminal cell is accepted");
	check_cursor_rectangle(fb, 1912, 1184, 8, 16, base ^ CURSOR_MASK,
	    base ^ CURSOR_MASK,
	    "edge cursor updates the complete shadow rectangle",
	    "edge cursor maps the complete rectangle to physical edge");
	ca.action = VIS_HIDE_CURSOR;
	bitmap_polled_cursor(arg, &ca);

	softc->mode = KD_GRAPHICS;
	ca = make_cursor(VIS_DISPLAY_CURSOR, 40, 64, 8, 16);
	flush_count = 0;
	bitmap_polled_cursor(arg, &ca);
	check_cursor_rectangle(fb, 40, 64, 8, 16, base ^ CURSOR_MASK, base,
	    "graphics-mode cursor updates every shadow pixel",
	    "graphics-mode cursor leaves every physical pixel unchanged");
	check(flush_count == 0,
	    "graphics-mode cursor callback intentionally skips physical flush");
	ca.action = VIS_HIDE_CURSOR;
	bitmap_polled_cursor(arg, &ca);
	check_cursor_rectangle(fb, 40, 64, 8, 16, base, base,
	    "graphics-mode hide restores every shadow pixel",
	    "graphics-mode hide leaves physical pixels unchanged");
	softc->mode = KD_TEXT;

	ca = make_cursor(VIS_DISPLAY_CURSOR, -1, 32, 8, 16);
	expect_invalid_cursor(softc, ca, "negative cursor column is rejected");
	ca = make_cursor(VIS_DISPLAY_CURSOR, 16, -1, 8, 16);
	expect_invalid_cursor(softc, ca, "negative cursor row is rejected");
	ca = make_cursor(VIS_DISPLAY_CURSOR, 16, 32, -1, 16);
	expect_invalid_cursor(softc, ca, "negative cursor width is rejected");
	ca = make_cursor(VIS_DISPLAY_CURSOR, 1919, 32, 2, 16);
	expect_invalid_cursor(softc, ca, "cursor crossing screen edge is rejected");
	ca = make_cursor(VIS_DISPLAY_CURSOR, 1912, 32, 9, 16);
	expect_invalid_cursor(softc, ca, "cursor crossing terminal edge is rejected");
	ca = make_cursor(VIS_HIDE_CURSOR, 16, 32, 0, 16);
	expect_invalid_cursor(softc, ca, "invalid hide leaves cursor state unchanged");
}

static void
test_get_cursor_callback(struct gfxp_fb_softc *softc)
{
	struct vis_conscursor ca;
	fb_info_t *fb = &softc->console->fb;
	uint64_t shadow_before, shadow_after;
	uint64_t physical_before, physical_after;
	uint32_t flushes_before;

	/* Establish a known visible B position independently of scenario A. */
	ca = make_cursor(VIS_DISPLAY_CURSOR, 24, 48, 8, 16);
	bitmap_polled_cursor((struct vis_polledio_arg *)softc, &ca);
	ca = make_cursor(VIS_GET_CURSOR, 0, 0, 0, 0);
	shadow_before = buffer_checksum(fb->fb, LOGICAL_BYTES, NULL, 0);
	physical_before = buffer_checksum(physical_memory, PHYSICAL_BYTES,
	    NULL, 0);
	flushes_before = flush_count;
	bitmap_cons_cursor(softc, &ca);
	shadow_after = buffer_checksum(fb->fb, LOGICAL_BYTES, NULL, 0);
	physical_after = buffer_checksum(physical_memory, PHYSICAL_BYTES,
	    NULL, 0);
	printf("VIS_GET_CURSOR_SHADOW_CHECKSUM_BEFORE=0x%016llx\n",
	    (unsigned long long)shadow_before);
	printf("VIS_GET_CURSOR_SHADOW_CHECKSUM_AFTER=0x%016llx\n",
	    (unsigned long long)shadow_after);
	printf("VIS_GET_CURSOR_PHYSICAL_CHECKSUM_BEFORE=0x%016llx\n",
	    (unsigned long long)physical_before);
	printf("VIS_GET_CURSOR_PHYSICAL_CHECKSUM_AFTER=0x%016llx\n",
	    (unsigned long long)physical_after);
	check(ca.col == 24 && ca.row == 48,
	    "VIS_GET_CURSOR returns saved cursor B coordinates");
	check(shadow_after == shadow_before,
	    "VIS_GET_CURSOR leaves the complete shadow surface unchanged");
	check(physical_after == physical_before,
	    "VIS_GET_CURSOR leaves the complete physical surface unchanged");
	check(flush_count == flushes_before,
	    "VIS_GET_CURSOR does not flush");
	ca = make_cursor(VIS_HIDE_CURSOR, 24, 48, 8, 16);
	bitmap_polled_cursor((struct vis_polledio_arg *)softc, &ca);
}

static void
test_exploratory_direct_display_relocation(struct gfxp_fb_softc *softc)
{
	struct vis_polledio_arg *arg = (struct vis_polledio_arg *)softc;
	fb_info_t *fb = &softc->console->fb;
	struct vis_conscursor ca;
	uint32_t old_shadow, old_physical, new_shadow, new_physical;
	uint32_t x, y;
	int old_shadow_all = 1, old_physical_all = 1;
	int new_shadow_all = 1, new_physical_all = 1;

	ca = make_cursor(VIS_DISPLAY_CURSOR, 80, 80, 8, 16);
	bitmap_polled_cursor(arg, &ca);
	ca = make_cursor(VIS_DISPLAY_CURSOR, 88, 96, 8, 16);
	bitmap_polled_cursor(arg, &ca);
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 8; x++) {
			if (logical_pixel(fb, 80 + x, 80 + y) !=
			    (BASE_PIXEL ^ CURSOR_MASK))
				old_shadow_all = 0;
			if (physical_pixel(80 + x, 80 + y) !=
			    (BASE_PIXEL ^ CURSOR_MASK))
				old_physical_all = 0;
			if (logical_pixel(fb, 88 + x, 96 + y) !=
			    (BASE_PIXEL ^ CURSOR_MASK))
				new_shadow_all = 0;
			if (physical_pixel(88 + x, 96 + y) !=
			    (BASE_PIXEL ^ CURSOR_MASK))
				new_physical_all = 0;
		}
	}
	old_shadow = logical_pixel(fb, 80, 80);
	old_physical = physical_pixel(80, 80);
	new_shadow = logical_pixel(fb, 88, 96);
	new_physical = physical_pixel(88, 96);
	printf("EXPLORATORY_DISPLAY_A_TO_B_OLD_SHADOW=0x%08x\n",
	    old_shadow);
	printf("EXPLORATORY_DISPLAY_A_TO_B_OLD_PHYSICAL=0x%08x\n",
	    old_physical);
	printf("EXPLORATORY_DISPLAY_A_TO_B_NEW_SHADOW=0x%08x\n",
	    new_shadow);
	printf("EXPLORATORY_DISPLAY_A_TO_B_NEW_PHYSICAL=0x%08x\n",
	    new_physical);
	printf("EXPLORATORY_DISPLAY_A_TO_B_RECTANGLES=old_shadow:%s,old_physical:%s,new_shadow:%s,new_physical:%s\n",
	    old_shadow_all ? "CURSOR" : "OTHER",
	    old_physical_all ? "CURSOR" : "OTHER",
	    new_shadow_all ? "CURSOR" : "OTHER",
	    new_physical_all ? "CURSOR" : "OTHER");
	if (old_shadow_all && old_physical_all &&
	    new_shadow_all && new_physical_all) {
		puts("EXPLORATORY_DISPLAY_A_TO_B_RESULT=STALE_OLD_CURSOR_REMAINS");
	} else {
		puts("EXPLORATORY_DISPLAY_A_TO_B_RESULT=OTHER_OBSERVED_BEHAVIOR");
	}
	/* Clear the exploratory residue only after recording its readback. */
	ca = make_cursor(VIS_HIDE_CURSOR, 88, 96, 8, 16);
	bitmap_polled_cursor(arg, &ca);
	for (y = 80; y < 96; y++) {
		for (x = 80; x < 88; x++)
			set_logical_pixel(fb, x, y, BASE_PIXEL);
	}
	rotfb_runtime_flush_rect(fb, 80, 80, 8, 16);
}

static void
set_copy_pattern(fb_info_t *fb, uint32_t x, uint32_t y, uint32_t seed)
{
	set_logical_pixel(fb, x, y, seed + 1);
	set_logical_pixel(fb, x + 1, y, seed + 2);
	set_logical_pixel(fb, x, y + 1, seed + 3);
	set_logical_pixel(fb, x + 1, y + 1, seed + 4);
}

static void
check_copy_rectangle(fb_info_t *fb, uint32_t x, uint32_t y,
    const uint32_t expected[4], const char *name)
{
	check(logical_pixel(fb, x, y) == expected[0] &&
	    logical_pixel(fb, x + 1, y) == expected[1] &&
	    logical_pixel(fb, x, y + 1) == expected[2] &&
	    logical_pixel(fb, x + 1, y + 1) == expected[3], name);
	check(physical_pixel(x, y) == expected[0] &&
	    physical_pixel(x + 1, y) == expected[1] &&
	    physical_pixel(x, y + 1) == expected[2] &&
	    physical_pixel(x + 1, y + 1) == expected[3],
	    "text-mode copy publishes destination to physical framebuffer");
}

static struct vis_conscopy
make_copy(short sx, short sy, short ex, short ey, short tx, short ty)
{
	struct vis_conscopy ma;

	ma.s_col = sx;
	ma.s_row = sy;
	ma.e_col = ex;
	ma.e_row = ey;
	ma.t_col = tx;
	ma.t_row = ty;
	return (ma);
}

static void check_guards(void);

static void
check_copy_pixel_both(fb_info_t *fb, uint32_t x, uint32_t y,
    uint32_t expected, const char *name)
{
	check(logical_pixel(fb, x, y) == expected &&
	    physical_pixel(x, y) == expected, name);
}

static void
test_copy_callback(struct gfxp_fb_softc *softc)
{
	fb_info_t *fb = &softc->console->fb;
	struct vis_conscopy ma;
	uint32_t expected[4];
	uint64_t checksum;
	uint32_t barriers;

	(void) memset(fb->fb, 0, LOGICAL_BYTES);
	rotfb_runtime_flush_all(fb);
	flush_count = 0;
	move_count = 0;

	set_copy_pattern(fb, 4, 4, 0x1000);
	expected[0] = 0x1001;
	expected[1] = 0x1002;
	expected[2] = 0x1003;
	expected[3] = 0x1004;
	ma = make_copy(4, 4, 5, 5, 20, 20);
	bitmap_cons_copy(softc, &ma);
	check(move_count == 2,
	    "KD_TEXT production copy moves each row exactly once");
	check(flush_count == 1,
	    "KD_TEXT production copy invokes one runtime rectangle flush");
	check_copy_rectangle(fb, 20, 20, expected,
	    "non-overlapping production callback copy is correct");

	set_copy_pattern(fb, 5, 5, 0x2000);
	expected[0] = 0x2001;
	expected[1] = 0x2002;
	expected[2] = 0x2003;
	expected[3] = 0x2004;
	ma = make_copy(5, 5, 6, 6, 5, 6);
	move_count = 0;
	flush_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 2,
	    "downward overlapping copy uses one move per row");
	check(flush_count == 1,
	    "downward overlapping copy flushes once");
	check(logical_pixel(fb, 5, 6) == expected[0] &&
	    logical_pixel(fb, 6, 6) == expected[1] &&
	    logical_pixel(fb, 5, 7) == expected[2] &&
	    logical_pixel(fb, 6, 7) == expected[3],
	    "downward overlapping copy preserves source row order");
	check(physical_pixel(5, 6) == expected[0] &&
	    physical_pixel(6, 6) == expected[1] &&
	    physical_pixel(5, 7) == expected[2] &&
	    physical_pixel(6, 7) == expected[3],
	    "downward overlapping copy reaches physical surface");

	set_copy_pattern(fb, 5, 6, 0x3000);
	expected[0] = 0x3001;
	expected[1] = 0x3002;
	expected[2] = 0x3003;
	expected[3] = 0x3004;
	/* Seed and publish surrounding sentinels before the overlapping move. */
	set_logical_pixel(fb, 4, 5, 0x30a1);
	set_logical_pixel(fb, 7, 5, 0x30a2);
	set_logical_pixel(fb, 4, 6, 0x30a3);
	set_logical_pixel(fb, 7, 6, 0x30a4);
	set_logical_pixel(fb, 4, 7, 0x30a5);
	set_logical_pixel(fb, 7, 7, 0x30a6);
	set_logical_pixel(fb, 5, 4, 0x30a7);
	set_logical_pixel(fb, 6, 4, 0x30a8);
	set_logical_pixel(fb, 5, 8, 0x30a9);
	set_logical_pixel(fb, 6, 8, 0x30aa);
	rotfb_runtime_flush_rect(fb, 4, 4, 4, 5);
	ma = make_copy(5, 6, 6, 7, 5, 5);
	move_count = 0;
	flush_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 2 && flush_count == 1,
	    "upward overlapping copy calls two production row moves and one production flush");
	check_copy_rectangle(fb, 5, 5, expected,
	    "upward overlapping copy matches shadow and physical readback");
	printf("UPWARD_COPY_SHADOW_READBACK=0x%08x,0x%08x,0x%08x,0x%08x\n",
	    logical_pixel(fb, 5, 5), logical_pixel(fb, 6, 5),
	    logical_pixel(fb, 5, 6), logical_pixel(fb, 6, 6));
	printf("UPWARD_COPY_PHYSICAL_READBACK=0x%08x,0x%08x,0x%08x,0x%08x\n",
	    physical_pixel(5, 5), physical_pixel(6, 5),
	    physical_pixel(5, 6), physical_pixel(6, 6));
	check_copy_pixel_both(fb, 5, 7, expected[2],
	    "upward overlapping copy preserves the original lower source row");
	check_copy_pixel_both(fb, 6, 7, expected[3],
	    "upward overlapping copy preserves the original lower source row (second pixel)");
	check_copy_pixel_both(fb, 4, 5, 0x30a1,
	    "upward overlapping copy preserves left neighbor row 5");
	check_copy_pixel_both(fb, 7, 5, 0x30a2,
	    "upward overlapping copy preserves right neighbor row 5");
	check_copy_pixel_both(fb, 4, 6, 0x30a3,
	    "upward overlapping copy preserves left neighbor row 6");
	check_copy_pixel_both(fb, 7, 6, 0x30a4,
	    "upward overlapping copy preserves right neighbor row 6");
	check_copy_pixel_both(fb, 4, 7, 0x30a5,
	    "upward overlapping copy preserves left neighbor row 7");
	check_copy_pixel_both(fb, 7, 7, 0x30a6,
	    "upward overlapping copy preserves right neighbor row 7");
	check_copy_pixel_both(fb, 5, 4, 0x30a7,
	    "upward overlapping copy preserves upper neighbor");
	check_copy_pixel_both(fb, 6, 4, 0x30a8,
	    "upward overlapping copy preserves upper neighbor (second pixel)");
	check_copy_pixel_both(fb, 5, 8, 0x30a9,
	    "upward overlapping copy preserves lower neighbor");
	check_copy_pixel_both(fb, 6, 8, 0x30aa,
	    "upward overlapping copy preserves lower neighbor (second pixel)");
	check_guards();

	set_logical_pixel(fb, 5, 8, 0x4001);
	set_logical_pixel(fb, 6, 8, 0x4002);
	ma = make_copy(5, 8, 6, 8, 6, 8);
	move_count = 0;
	flush_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 1 && flush_count == 1,
	    "horizontal overlap uses one memmove and one flush");
	check(logical_pixel(fb, 6, 8) == 0x4001 &&
	    logical_pixel(fb, 7, 8) == 0x4002,
	    "horizontal overlap uses memmove semantics");
	check(physical_pixel(6, 8) == 0x4001 &&
	    physical_pixel(7, 8) == 0x4002,
	    "horizontal overlap is synchronized physically");

	set_copy_pattern(fb, 40, 40, 0x5000);
	expected[0] = 0x5001;
	expected[1] = 0x5002;
	expected[2] = 0x5003;
	expected[3] = 0x5004;
	ma = make_copy(40, 40, 41, 41, 60, 60);
	check(physical_pixel(60, 60) == 0,
	    "graphics-mode destination starts clear");
	softc->mode = KD_GRAPHICS;
	move_count = 0;
	flush_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 2,
	    "KD_GRAPHICS production copy updates each shadow row once");
	check(flush_count == 0,
	    "KD_GRAPHICS production copy does not flush physical surface");
	check(logical_pixel(fb, 60, 60) == expected[0] &&
	    logical_pixel(fb, 61, 60) == expected[1] &&
	    logical_pixel(fb, 60, 61) == expected[2] &&
	    logical_pixel(fb, 61, 61) == expected[3],
	    "KD_GRAPHICS production copy updates shadow framebuffer");
	check(physical_pixel(60, 60) == 0,
	    "KD_GRAPHICS production copy leaves physical surface unchanged");
	softc->mode = KD_TEXT;

	ma = make_copy(-1, 4, 0, 5, 80, 80);
	checksum = buffer_checksum(fb->fb, LOGICAL_BYTES,
	    physical_memory, PHYSICAL_BYTES);
	move_count = 0;
	flush_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 0 && flush_count == 0 &&
	    buffer_checksum(fb->fb, LOGICAL_BYTES, physical_memory,
	    PHYSICAL_BYTES) == checksum,
	    "negative source coordinates are rejected without writes");

	ma = make_copy(4, 4, 3, 5, 80, 80);
	move_count = 0;
	flush_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 0 && flush_count == 0,
	    "reversed source rectangle is rejected");

	ma = make_copy(4, 4, 5, 5, 1919, 80);
	checksum = buffer_checksum(fb->fb, LOGICAL_BYTES,
	    physical_memory, PHYSICAL_BYTES);
	move_count = 0;
	flush_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 0 && flush_count == 0 &&
	    buffer_checksum(fb->fb, LOGICAL_BYTES, physical_memory,
	    PHYSICAL_BYTES) == checksum,
	    "destination crossing screen edge is rejected atomically");

	ma = make_copy(4, 4, 5, 5, -1, 80);
	barriers = flush_count;
	move_count = 0;
	bitmap_cons_copy(softc, &ma);
	check(move_count == 0 && flush_count == barriers,
	    "negative target coordinate is rejected");
}

typedef struct callback_race_arg {
	struct gfxp_fb_softc *softc;
	struct vis_conscopy request;
} callback_race_arg_t;

typedef struct fini_race_arg {
	fb_info_t *fb;
	int result;
	int done;
} fini_race_arg_t;

static void *
run_blocked_copy_callback(void *arg)
{
	callback_race_arg_t *a = arg;

	bitmap_polled_copy((struct vis_polledio_arg *)a->softc,
	    &a->request);
	return (NULL);
}

static void *
run_fini_thread(void *arg)
{
	fini_race_arg_t *a = arg;

	a->result = rotfb_runtime_fini(a->fb);
	__atomic_store_n(&a->done, 1, __ATOMIC_RELEASE);
	return (NULL);
}

static void
test_runtime_lifecycle_race_and_recovery(struct gfxp_fb_softc *softc)
{
	fb_info_t *fb = &softc->console->fb;
	callback_race_arg_t callback_arg;
	fini_race_arg_t fini_arg;
	pthread_t callback_thread, fini_thread;
	struct vis_conscopy stale_request;
	uint64_t restored_checksum;
	unsigned int maps_before, unmaps_before, allocs_before, frees_before;
	uint32_t moves_before;
	int gate_closed = 0;
	unsigned int i;

	maps_before = map_calls;
	allocs_before = alloc_calls;
	check(rotfb_runtime_init(fb) == DDI_SUCCESS,
	    "runtime init is idempotent while the same lifecycle is ready");
	check(map_calls == maps_before && alloc_calls == allocs_before,
	    "idempotent runtime init allocates and maps nothing");

	set_copy_pattern(fb, 100, 100, 0x6000);
	callback_arg.softc = softc;
	callback_arg.request = make_copy(100, 100, 101, 101, 120, 120);
	fini_arg.fb = fb;
	fini_arg.result = DDI_FAILURE;
	fini_arg.done = 0;
	__atomic_store_n(&memmove_blocked, 0, __ATOMIC_RELEASE);
	__atomic_store_n(&release_memmove, 0, __ATOMIC_RELEASE);
	__atomic_store_n(&block_next_memmove, 1, __ATOMIC_RELEASE);

	check(pthread_create(&callback_thread, NULL,
	    run_blocked_copy_callback, &callback_arg) == 0,
	    "start real production polled copy callback thread");
	for (i = 0; i < 10000000; i++) {
		if (__atomic_load_n(&memmove_blocked, __ATOMIC_ACQUIRE))
			break;
		(void) sched_yield();
	}
	check(__atomic_load_n(&memmove_blocked, __ATOMIC_ACQUIRE),
	    "production callback reaches the controlled memmove hold point");

	unmaps_before = unmap_calls;
	frees_before = free_calls;
	check(pthread_create(&fini_thread, NULL, run_fini_thread,
	    &fini_arg) == 0, "start runtime fini while production callback is active");
	for (i = 0; i < 10000000; i++) {
		int entered = rotfb_runtime_callback_enter();

		if (entered < 0) {
			gate_closed = 1;
			break;
		}
		if (entered > 0)
			rotfb_runtime_callback_exit(entered);
		(void) sched_yield();
	}
	check(gate_closed, "fini closes callback gate during active production copy");
	check(!__atomic_load_n(&fini_arg.done, __ATOMIC_ACQUIRE),
	    "fini waits while the real production callback holds its reference");
	check(unmap_calls == unmaps_before && free_calls == frees_before,
	    "fini does not unmap WC memory or free shadow before callback exit");

	__atomic_store_n(&release_memmove, 1, __ATOMIC_RELEASE);
	(void) pthread_join(callback_thread, NULL);
	(void) pthread_join(fini_thread, NULL);
	check(fini_arg.result == DDI_SUCCESS,
	    "fini succeeds after the production callback exits");
	check(__atomic_load_n(&fini_arg.done, __ATOMIC_ACQUIRE),
	    "fini thread completes after callback drain");
	check(!rotfb_info.runtime_ready && rotfb_info.boot_ready &&
	    fb->fb == boot_memory && fb->shadow_fb == NULL &&
	    fb->physical.fb == physical_memory &&
	    rotfb_info.logical_fb == boot_memory &&
	    rotfb_info.physical_fb == physical_memory,
	    "fini restores all boot framebuffer pointers and readiness");
	check(unmap_calls == unmaps_before + 1 &&
	    free_calls == frees_before + 1,
	    "fini releases exactly one WC mapping and one runtime shadow");

	/* A stale polled callback is rejected after fini, before softc access. */
	stale_request = make_copy(100, 100, 101, 101, 120, 120);
	moves_before = move_count;
	restored_checksum = buffer_checksum(boot_memory, LOGICAL_BYTES,
	    physical_memory, PHYSICAL_BYTES);
	bitmap_polled_copy((struct vis_polledio_arg *)softc, &stale_request);
	check(move_count == moves_before &&
	    buffer_checksum(boot_memory, LOGICAL_BYTES, physical_memory,
	    PHYSICAL_BYTES) == restored_checksum,
	    "stale production polled callback after fini is rejected without writes");
	check(rotfb_runtime_fini(fb) == DDI_FAILURE,
	    "direct repeated runtime fini reports already-finished lifecycle");
	check(unmap_calls == unmaps_before + 1 &&
	    free_calls == frees_before + 1,
	    "repeated fini does not double-unmap or double-free");

	/* Fault-inject both acquisition stages on a reattach after a fini. */
	maps_before = map_calls;
	allocs_before = alloc_calls;
	unmaps_before = unmap_calls;
	frees_before = free_calls;
	fail_map = 1;
	check(rotfb_runtime_init(fb) == DDI_FAILURE,
	    "reinit rejects WC mapping failure after a completed lifecycle");
	check(map_calls == maps_before + 1 && alloc_calls == allocs_before &&
	    unmap_calls == unmaps_before && free_calls == frees_before,
	    "failed WC remap leaves no allocation or unmap to perform");
	check(fb->fb == boot_memory && fb->shadow_fb == NULL &&
	    fb->physical.fb == physical_memory && !rotfb_info.runtime_ready &&
	    rotfb_info.boot_ready,
	    "failed WC remap leaves restored boot metadata intact");

	fail_map = 0;
	fail_alloc = 1;
	check(rotfb_runtime_init(fb) == DDI_FAILURE,
	    "reinit rejects shadow allocation failure after a completed lifecycle");
	check(map_calls == maps_before + 2 &&
	    alloc_calls == allocs_before + 1 &&
	    unmap_calls == unmaps_before + 1 && free_calls == frees_before,
	    "failed shadow allocation unmaps exactly once and frees no absent shadow");
	check(fb->fb == boot_memory && fb->shadow_fb == NULL &&
	    fb->physical.fb == physical_memory && !rotfb_info.runtime_ready &&
	    rotfb_info.boot_ready,
	    "failed shadow allocation preserves boot framebuffer metadata");

	fail_alloc = 0;
	check(rotfb_runtime_init(fb) == DDI_SUCCESS,
	    "reinit succeeds after map and allocation failures");
	check(rotfb_info.runtime_ready && fb->fb == fb->shadow_fb &&
	    fb->physical.fb == physical_memory,
	    "successful reinit republishes runtime shadow and WC mapping");
	check(rotfb_runtime_init(fb) == DDI_SUCCESS,
	    "second init on reinitialized runtime remains idempotent");
	check(map_calls == maps_before + 3 &&
	    alloc_calls == allocs_before + 2 &&
	    unmap_calls == unmaps_before + 1 &&
	    free_calls == frees_before,
	    "reinitialized runtime owns exactly one mapping and one shadow");

	check(rotfb_runtime_fini(fb) == DDI_SUCCESS,
	    "second lifecycle fini succeeds");
	check(!rotfb_info.runtime_ready && rotfb_info.boot_ready &&
	    fb->fb == boot_memory && fb->shadow_fb == NULL &&
	    fb->physical.fb == physical_memory,
	    "second lifecycle restores boot state");
	check(unmap_calls == unmaps_before + 2 &&
	    free_calls == frees_before + 1,
	    "second lifecycle releases its resources exactly once");
	check(rotfb_runtime_fini(fb) == DDI_FAILURE,
	    "second lifecycle repeated fini is safely rejected");
	check(unmap_calls == unmaps_before + 2 &&
	    free_calls == frees_before + 1,
	    "second repeated fini does not double-release resources");

	printf("RUNTIME_LIFECYCLE_RESOURCE_COUNTS=map:%u,unmap:%u,alloc:%u,free:%u\n",
	    map_calls, unmap_calls, alloc_calls, free_calls);
}

static void
check_guards(void)
{
	size_t i;
	unsigned int failures_before = failures;

	for (i = 0; i < GUARD_BYTES; i++) {
		check(physical_allocation[i] == GUARD_VALUE &&
		    physical_allocation[GUARD_BYTES + PHYSICAL_BYTES + i] ==
		    GUARD_VALUE, "physical framebuffer guard bytes remain intact");
		check(boot_allocation[i] == GUARD_VALUE &&
		    boot_allocation[GUARD_BYTES + LOGICAL_BYTES + i] ==
		    GUARD_VALUE, "boot framebuffer guard bytes remain intact");
		if (shadow_allocation != NULL) {
			check(shadow_allocation[i] == GUARD_VALUE &&
			    shadow_allocation[GUARD_BYTES + shadow_allocation_size +
			    i] == GUARD_VALUE,
			    "runtime shadow framebuffer guard bytes remain intact");
		}
	}
	printf("FRAMEBUFFER_GUARD_REGIONS=%s bytes_per_edge=%u\n",
	    failures == failures_before ? "PASS" : "FAIL", GUARD_BYTES);
}

int
main(void)
{
	fb_info_t fb;
	union gfx_console console;
	struct gfxp_fb_softc softc;
	uint32_t *pixels;
	size_t i;

	physical_allocation = malloc(PHYSICAL_BYTES + 2U * GUARD_BYTES);
	boot_allocation = malloc(LOGICAL_BYTES + 2U * GUARD_BYTES);
	if (physical_allocation == NULL || boot_allocation == NULL) {
		(void) fprintf(stderr, "FAIL: framebuffer test allocation\n");
		return (1);
	}
	(void) memset(physical_allocation, GUARD_VALUE,
	    PHYSICAL_BYTES + 2U * GUARD_BYTES);
	(void) memset(boot_allocation, GUARD_VALUE,
	    LOGICAL_BYTES + 2U * GUARD_BYTES);
	physical_memory = physical_allocation + GUARD_BYTES;
	boot_memory = boot_allocation + GUARD_BYTES;
	(void) memset(physical_memory, 0, PHYSICAL_BYTES);
	pixels = (uint32_t *)boot_memory;
	for (i = 0; i < LOGICAL_BYTES / sizeof (*pixels); i++)
		pixels[i] = BASE_PIXEL;

	(void) memset(&rotfb_info, 0, sizeof (rotfb_info));
	rotfb_info.physical_paddr = 0x100000;
	rotfb_info.physical_fb = physical_memory;
	rotfb_info.physical_size = PHYSICAL_BYTES;
	rotfb_info.physical_width = 1200;
	rotfb_info.physical_height = 1920;
	rotfb_info.physical_pitch = 4800;
	rotfb_info.physical_bpp = 4;
	rotfb_info.logical_fb = boot_memory;
	rotfb_info.logical_size = LOGICAL_BYTES;
	rotfb_info.logical_width = 1920;
	rotfb_info.logical_height = 1200;
	rotfb_info.logical_pitch = 7680;
	rotfb_info.configured = B_TRUE;
	rotfb_info.boot_ready = B_TRUE;

	(void) memset(&fb, 0, sizeof (fb));
	fb.fb = boot_memory;
	fb.fb_size = LOGICAL_BYTES;
	fb.pitch = 7680;
	fb.screen.x = 1920;
	fb.screen.y = 1200;
	fb.bpp = 4;
	fb.depth = 32;
	fb.physical.valid = B_TRUE;
	fb.physical.paddr = rotfb_info.physical_paddr;
	fb.physical.width = 1200;
	fb.physical.height = 1920;
	fb.physical.pitch = 4800;
	fb.physical.bpp = 4;
	fb.physical.size = PHYSICAL_BYTES;
	fb.physical.fb = physical_memory;
	fb.terminal_origin.x = 0;
	fb.terminal_origin.y = 0;
	fb.terminal.x = 240;
	fb.terminal.y = 75;
	fb.font_width = 8;
	fb.font_height = 16;
	fb.cursor.pos.x = 7;
	fb.cursor.pos.y = 8;
	fb.cursor.origin.x = 56;
	fb.cursor.origin.y = 128;
	fb.cursor.visible = B_FALSE;

	(void) memset(&console, 0, sizeof (console));
	console.fb = fb;
	(void) memset(&softc, 0, sizeof (softc));
	softc.mode = KD_TEXT;
	softc.console = &console;

	check(rotfb_runtime_init(&console.fb) == DDI_SUCCESS,
	    "production runtime initialization succeeds");
	check(console.fb.fb == console.fb.shadow_fb &&
	    console.fb.fb != boot_memory,
	    "test exercises distinct physical and runtime shadow surfaces");
	flush_count = 0;
	move_count = 0;

	{
		unsigned int before = failures;

		test_cursor_callbacks(&softc);
		check_guards();
		printf("SCENARIO_A_RESULT=%s failures=%u\n",
		    failures == before ? "PASS" : "FAIL", failures - before);
	}
	{
		unsigned int before = failures;

		test_get_cursor_callback(&softc);
		printf("SCENARIO_B_RESULT=%s failures=%u\n",
		    failures == before ? "PASS" : "FAIL", failures - before);
	}
	test_exploratory_direct_display_relocation(&softc);
	{
		unsigned int before = failures;

		test_copy_callback(&softc);
		check_guards();
		printf("SCENARIO_C_RESULT=%s failures=%u\n",
		    failures == before ? "PASS" : "FAIL", failures - before);
	}

	{
		unsigned int before = failures;

		test_runtime_lifecycle_race_and_recovery(&softc);
		check_guards();
		printf("SCENARIO_D_RESULT=%s failures=%u\n",
		    failures == before ? "PASS" : "FAIL", failures - before);
	}
	check(console.fb.fb == boot_memory && console.fb.shadow_fb == NULL &&
	    !rotfb_info.runtime_ready && rotfb_info.boot_ready,
	    "all runtime lifecycle scenarios finish in restored boot state");

	free(physical_allocation);
	free(boot_allocation);
	if (failures != 0) {
		(void) fprintf(stderr, "ROTFB production bitmap callback tests: %u failures\n",
		    failures);
		return (1);
	}
	(void) puts("ROTFB production bitmap callback tests: PASS");
	return (0);
}
