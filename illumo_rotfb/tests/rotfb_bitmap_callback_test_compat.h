#ifndef ROTFB_BITMAP_CALLBACK_TEST_COMPAT_H
#define ROTFB_BITMAP_CALLBACK_TEST_COMPAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef int boolean_t;
#define B_TRUE 1
#define B_FALSE 0
#define DDI_SUCCESS 0
#define DDI_FAILURE (-1)
#define KM_NOSLEEP 1
#define GFXP_MEMORY_WRITECOMBINED 1
#define KD_TEXT 0
#define KD_GRAPHICS 1
#define VIS_HIDE_CURSOR 0
#define VIS_DISPLAY_CURSOR 1
#define VIS_GET_CURSOR 2

typedef void *gfxp_kva_t;

typedef struct fb_info_pixel_coord {
	uint16_t x;
	uint16_t y;
} fb_info_pixel_coord_t;

typedef struct fb_info_char_coord {
	uint16_t x;
	uint16_t y;
} fb_info_char_coord_t;

typedef struct fb_info_physical {
	boolean_t valid;
	uint64_t paddr;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t bpp;
	uint64_t size;
	uint8_t *fb;
} fb_info_physical_t;

typedef struct fb_cursor {
	fb_info_pixel_coord_t origin;
	fb_info_char_coord_t pos;
	boolean_t visible;
} fb_cursor_t;

typedef struct fb_info {
	uint8_t *fb;
	uint8_t *shadow_fb;
	uint64_t fb_size;
	uint32_t pitch;
	fb_info_pixel_coord_t screen;
	uint8_t bpp;
	uint8_t depth;
	fb_info_physical_t physical;
	fb_info_pixel_coord_t terminal_origin;
	fb_info_char_coord_t terminal;
	fb_cursor_t cursor;
	uint16_t font_width;
	uint16_t font_height;
} fb_info_t;

typedef union color {
	uint8_t mono;
	uint8_t four;
	uint8_t eight;
	uint8_t sixteen[2];
	uint8_t twentyfour[3];
	uint8_t thirtytwo[4];
} color_t;

typedef short screen_pos_t;
typedef short screen_size_t;

struct vis_conscursor {
	screen_pos_t row;
	screen_pos_t col;
	screen_size_t width;
	screen_size_t height;
	color_t fg_color;
	color_t bg_color;
	short action;
};

struct vis_conscopy {
	screen_pos_t s_row;
	screen_pos_t s_col;
	screen_pos_t e_row;
	screen_pos_t e_col;
	screen_pos_t t_row;
	screen_pos_t t_col;
};

struct vis_polledio_arg {
	int unused;
};

union gfx_console {
	fb_info_t fb;
};

struct gfxp_fb_softc {
	int mode;
	union gfx_console *console;
};

uint32_t atomic_add_32_nv(volatile uint32_t *, int32_t);
uint32_t atomic_cas_32(volatile uint32_t *, uint32_t, uint32_t);
uint32_t atomic_dec_32(volatile uint32_t *);
uint32_t atomic_swap_32(volatile uint32_t *, uint32_t);
void membar_producer(void);
void delay(int);
void *kmem_zalloc(size_t, int);
void kmem_free(void *, size_t);
void *gfxp_map_kernel_space(uint64_t, size_t, int);
void gfxp_unmap_kernel_space(gfxp_kva_t, size_t);

#endif
