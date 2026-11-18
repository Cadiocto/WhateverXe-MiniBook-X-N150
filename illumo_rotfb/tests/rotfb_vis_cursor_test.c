#include <stdio.h>
#include <string.h>

#include "../rotfb_vis_cursor.h"

typedef struct cursor_state {
	unsigned short pos_x;
	unsigned short pos_y;
	unsigned short origin_x;
	unsigned short origin_y;
	int visible;
} cursor_state_t;

static rotfb_vis_cursor_geometry_t
good_geometry(void)
{
	rotfb_vis_cursor_geometry_t g;

	g.screen_width = 1920;
	g.screen_height = 1200;
	g.terminal_origin_x = 0;
	g.terminal_origin_y = 0;
	g.terminal_width = 240;
	g.terminal_height = 75;
	g.font_width = 8;
	g.font_height = 16;
	g.bpp = 4;
	g.pitch = 7680;
	g.fb_size = 7680ULL * 1200ULL;
	return (g);
}

static int
expect_reject(const char *name, const rotfb_vis_cursor_geometry_t *g,
    int col, int row, int width, int height)
{
	cursor_state_t state = { 11, 12, 13, 14, 1 };
	cursor_state_t before = state;
	int rc;

	rc = rotfb_vis_cursor_update(g, col, row, width, height,
	    &state.pos_x, &state.pos_y, &state.origin_x, &state.origin_y);
	if (rc != 0 || memcmp(&state, &before, sizeof (state)) != 0) {
		(void) fprintf(stderr, "FAIL: %s changed cursor state or was accepted\n",
		    name);
		return (1);
	}
	return (0);
}

int
main(void)
{
	rotfb_vis_cursor_geometry_t g = good_geometry();
	rotfb_vis_cursor_geometry_t bad;
	cursor_state_t state = { 99, 98, 97, 96, 0 };
	int failures = 0;

	if (!rotfb_vis_cursor_update(&g, 8, 16, 8, 16,
	    &state.pos_x, &state.pos_y, &state.origin_x, &state.origin_y) ||
	    state.pos_x != 1 || state.pos_y != 1 ||
	    state.origin_x != 8 || state.origin_y != 16 || state.visible != 0) {
		(void) fprintf(stderr, "FAIL: valid cursor update\n");
		failures++;
	}

	failures += expect_reject("negative column", &g, -1, 16, 8, 16);
	failures += expect_reject("negative row", &g, 8, -1, 8, 16);
	failures += expect_reject("zero width", &g, 8, 16, 0, 16);
	failures += expect_reject("zero height", &g, 8, 16, 8, 0);
	failures += expect_reject("negative width", &g, 8, 16, -1, 16);
	failures += expect_reject("negative height", &g, 8, 16, 8, -1);
	failures += expect_reject("column outside screen", &g, 1920, 16, 8, 16);
	failures += expect_reject("row outside screen", &g, 8, 1200, 8, 16);
	failures += expect_reject("width past screen", &g, 1919, 16, 2, 16);
	failures += expect_reject("height past screen", &g, 8, 1199, 8, 2);

	bad = g;
	bad.terminal_origin_x = 16;
	failures += expect_reject("column before terminal origin", &bad, 8, 16, 8, 16);
	bad = g;
	bad.font_width = 0;
	failures += expect_reject("zero font width", &bad, 8, 16, 8, 16);
	bad = g;
	bad.font_height = 0;
	failures += expect_reject("zero font height", &bad, 8, 16, 8, 16);
	bad = g;
	bad.pitch = 7679;
	failures += expect_reject("short pitch", &bad, 8, 16, 8, 16);
	bad = g;
	bad.fb_size--;
	failures += expect_reject("short framebuffer", &bad, 8, 16, 8, 16);
	bad = g;
	bad.pitch = ~0ULL;
	failures += expect_reject("pitch multiplication overflow", &bad, 8, 16, 8, 16);

	if (failures != 0)
		return (1);
	(void) puts("ROTFB VIS cursor C tests: PASS");
	return (0);
}
