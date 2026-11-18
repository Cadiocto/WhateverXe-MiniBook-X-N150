#ifndef _SYS_ROTFB_VIS_CURSOR_H
#define _SYS_ROTFB_VIS_CURSOR_H

/*
 * Portable VIS cursor validation shared by gfxp_bitmap and the host C tests.
 * VIS coordinates and dimensions are signed short values. They must be
 * rejected before any conversion to unsigned framebuffer coordinates.
 */
typedef struct rotfb_vis_cursor_geometry {
	unsigned int screen_width;
	unsigned int screen_height;
	unsigned int terminal_origin_x;
	unsigned int terminal_origin_y;
	unsigned int terminal_width;
	unsigned int terminal_height;
	unsigned int font_width;
	unsigned int font_height;
	unsigned int bpp;
	unsigned long long pitch;
	unsigned long long fb_size;
} rotfb_vis_cursor_geometry_t;

static inline int
rotfb_vis_cursor_validate(const rotfb_vis_cursor_geometry_t *g,
    int col, int row, int width, int height)
{
	const unsigned long long max_value = ~0ULL;
	unsigned long long min_pitch, frame_size, row_bytes;
	unsigned long long row_offset, col_offset, offset, available;
	unsigned long long terminal_width_px, terminal_height_px;
	unsigned int relative_x, relative_y;

	/* Reject signed VIS values before any unsigned conversion. */
	if (g == 0 || col < 0 || row < 0 || width <= 0 || height <= 0)
		return (0);
	if (g->screen_width == 0 || g->screen_height == 0 ||
	    g->terminal_width == 0 || g->terminal_height == 0 ||
	    g->font_width == 0 || g->font_height == 0 ||
	    g->bpp == 0 || g->pitch == 0 || g->fb_size == 0)
		return (0);
	if (g->terminal_origin_x >= g->screen_width ||
	    g->terminal_origin_y >= g->screen_height ||
	    (unsigned int)col < g->terminal_origin_x ||
	    (unsigned int)row < g->terminal_origin_y ||
	    (unsigned int)col >= g->screen_width ||
	    (unsigned int)row >= g->screen_height)
		return (0);
	if ((unsigned int)width > g->screen_width - (unsigned int)col ||
	    (unsigned int)height > g->screen_height - (unsigned int)row)
		return (0);

	relative_x = (unsigned int)col - g->terminal_origin_x;
	relative_y = (unsigned int)row - g->terminal_origin_y;
	terminal_width_px = (unsigned long long)g->terminal_width *
	    g->font_width;
	terminal_height_px = (unsigned long long)g->terminal_height *
	    g->font_height;
	if ((unsigned long long)relative_x >= terminal_width_px ||
	    (unsigned long long)relative_y >= terminal_height_px ||
	    (unsigned long long)(unsigned int)width >
	    terminal_width_px - relative_x ||
	    (unsigned long long)(unsigned int)height >
	    terminal_height_px - relative_y)
		return (0);

	min_pitch = (unsigned long long)g->screen_width * g->bpp;
	if (g->pitch < min_pitch ||
	    g->pitch > max_value / g->screen_height)
		return (0);
	frame_size = g->pitch * g->screen_height;
	if (g->fb_size < frame_size)
		return (0);
	if ((unsigned long long)(unsigned int)width > max_value / g->bpp ||
	    (unsigned long long)(unsigned int)row > max_value / g->pitch ||
	    (unsigned long long)(unsigned int)col > max_value / g->bpp)
		return (0);
	row_bytes = (unsigned long long)(unsigned int)width * g->bpp;
	if (row_bytes > g->pitch)
		return (0);
	row_offset = (unsigned long long)(unsigned int)row * g->pitch;
	col_offset = (unsigned long long)(unsigned int)col * g->bpp;
	if (row_offset > max_value - col_offset)
		return (0);
	offset = row_offset + col_offset;
	if (offset > g->fb_size || row_bytes > g->fb_size - offset)
		return (0);
	available = g->fb_size - offset - row_bytes;
	if ((unsigned long long)(unsigned int)(height - 1) >
	    available / g->pitch)
		return (0);
	return (1);
}

static inline int
rotfb_vis_cursor_update(const rotfb_vis_cursor_geometry_t *g,
    int col, int row, int width, int height,
    unsigned short *pos_x, unsigned short *pos_y,
    unsigned short *origin_x, unsigned short *origin_y)
{
	unsigned int relative_x, relative_y, new_x, new_y;

	if (pos_x == 0 || pos_y == 0 || origin_x == 0 || origin_y == 0 ||
	    !rotfb_vis_cursor_validate(g, col, row, width, height))
		return (0);
	relative_x = (unsigned int)col - g->terminal_origin_x;
	relative_y = (unsigned int)row - g->terminal_origin_y;
	new_x = relative_x / g->font_width;
	new_y = relative_y / g->font_height;
	if (new_x > 0xffffU || new_y > 0xffffU)
		return (0);

	/* Commit cursor position only after all validation has succeeded. */
	*pos_x = (unsigned short)new_x;
	*pos_y = (unsigned short)new_y;
	*origin_x = (unsigned short)col;
	*origin_y = (unsigned short)row;
	return (1);
}

#endif /* _SYS_ROTFB_VIS_CURSOR_H */
