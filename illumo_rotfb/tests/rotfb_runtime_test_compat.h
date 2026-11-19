#ifndef ROTFB_RUNTIME_TEST_COMPAT_H
#define ROTFB_RUNTIME_TEST_COMPAT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
typedef int boolean_t;
#define B_TRUE 1
#define B_FALSE 0
#define DDI_SUCCESS 0
#define DDI_FAILURE -1
#define KM_NOSLEEP 1
typedef void *gfxp_kva_t;
#define GFXP_MEMORY_WRITECOMBINED 1
typedef struct fb_info_pixel_coord { uint32_t x, y; } fb_info_pixel_coord_t;
typedef struct fb_info_physical {
	boolean_t valid;
	uint64_t paddr;
	uint32_t width, height, pitch, bpp;
	uint64_t size;
	uint8_t *fb;
} fb_info_physical_t;
typedef struct fb_info {
	uint8_t *fb;
	uint8_t *shadow_fb;
	uint64_t fb_size;
	uint32_t pitch;
	fb_info_pixel_coord_t screen;
	uint8_t bpp, depth;
	fb_info_physical_t physical;
} fb_info_t;
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
