#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/framebuffer.h>
#include <sys/gfx_private.h>
#include <sys/rotfb.h>

static uint8_t *rotfb_runtime_mapping;
static size_t rotfb_runtime_mapping_size;
static uint8_t *rotfb_runtime_shadow;
static size_t rotfb_runtime_shadow_size;
static fb_info_t *rotfb_runtime_owner;

typedef struct rotfb_saved_state {
	uint8_t *fb;
	uint8_t *shadow_fb;
	uint8_t *physical_fb;
	uint8_t *rotfb_physical_fb;
	uint8_t *rotfb_logical_fb;
	uint64_t fb_size;
	uint32_t pitch;
	fb_info_pixel_coord_t screen;
	uint8_t bpp;
	uint8_t depth;
	boolean_t valid;
} rotfb_saved_state_t;

static rotfb_saved_state_t rotfb_saved_state;

static boolean_t
rotfb_runtime_valid(void)
{
	uint64_t required_size;

	if (!rotfb_info.configured || !rotfb_info.boot_ready ||
	    rotfb_info.physical_fb == NULL || rotfb_info.logical_fb == NULL ||
	    rotfb_info.physical_width != ROTFB_PHYS_WIDTH ||
	    rotfb_info.physical_height != ROTFB_PHYS_HEIGHT ||
	    rotfb_info.physical_bpp != ROTFB_BYTES_PER_PIXEL ||
	    rotfb_info.logical_width != ROTFB_LOGICAL_WIDTH ||
	    rotfb_info.logical_height != ROTFB_LOGICAL_HEIGHT)
		return (B_FALSE);

	if (rotfb_info.physical_pitch <
	    rotfb_info.physical_width * ROTFB_BYTES_PER_PIXEL ||
	    (rotfb_info.physical_pitch % ROTFB_BYTES_PER_PIXEL) != 0)
		return (B_FALSE);

	if (rotfb_info.physical_height != 0 &&
	    (uint64_t)rotfb_info.physical_pitch >
	    (uint64_t)-1 / rotfb_info.physical_height)
		return (B_FALSE);

	required_size = (uint64_t)rotfb_info.physical_pitch *
	    rotfb_info.physical_height;
	if (rotfb_info.physical_size < required_size)
		return (B_FALSE);

	if (rotfb_info.logical_pitch <
	    rotfb_info.logical_width * ROTFB_BYTES_PER_PIXEL)
		return (B_FALSE);

	if (rotfb_info.logical_height != 0 &&
	    (uint64_t)rotfb_info.logical_pitch >
	    (uint64_t)-1 / rotfb_info.logical_height)
		return (B_FALSE);

	required_size = (uint64_t)rotfb_info.logical_pitch *
	    rotfb_info.logical_height;
	if (rotfb_info.logical_size < required_size)
		return (B_FALSE);

	return (B_TRUE);
}

int
rotfb_runtime_init(fb_info_t *fb)
{
	uint8_t *mapping, *shadow;
	size_t map_size, shadow_size;

	if (fb == NULL)
		return (DDI_FAILURE);

	if (rotfb_info.runtime_ready)
		return (rotfb_runtime_owner == fb ? DDI_SUCCESS : DDI_FAILURE);

	if (!rotfb_runtime_valid() || rotfb_info.physical_paddr == 0 ||
	    rotfb_info.physical_size == 0 ||
	    !fb->physical.valid ||
	    fb->physical.paddr != rotfb_info.physical_paddr ||
	    fb->physical.width != rotfb_info.physical_width ||
	    fb->physical.height != rotfb_info.physical_height ||
	    fb->physical.pitch != rotfb_info.physical_pitch ||
	    fb->physical.bpp != rotfb_info.physical_bpp ||
	    fb->physical.size != rotfb_info.physical_size ||
	    fb->physical.fb != rotfb_info.physical_fb ||
	    fb->fb != rotfb_info.logical_fb ||
	    fb->fb_size != rotfb_info.logical_size ||
	    fb->pitch != rotfb_info.logical_pitch ||
	    fb->screen.x != rotfb_info.logical_width ||
	    fb->screen.y != rotfb_info.logical_height ||
	    fb->bpp != ROTFB_BYTES_PER_PIXEL || fb->depth != 32)
		return (DDI_FAILURE);

	map_size = (size_t)rotfb_info.physical_size;
	shadow_size = (size_t)rotfb_info.logical_size;
	if ((uint64_t)map_size != rotfb_info.physical_size ||
	    (uint64_t)shadow_size != rotfb_info.logical_size)
		return (DDI_FAILURE);

	mapping = (uint8_t *)gfxp_map_kernel_space(rotfb_info.physical_paddr,
	    map_size, GFXP_MEMORY_WRITECOMBINED);
	if (mapping == NULL)
		return (DDI_FAILURE);

	shadow = kmem_zalloc(shadow_size, KM_SLEEP);
	if (shadow == NULL) {
		gfxp_unmap_kernel_space((gfxp_kva_t)mapping, map_size);
		return (DDI_FAILURE);
	}

	bcopy(fb->fb, shadow, shadow_size);

	rotfb_saved_state.fb = fb->fb;
	rotfb_saved_state.shadow_fb = fb->shadow_fb;
	rotfb_saved_state.fb_size = fb->fb_size;
	rotfb_saved_state.pitch = fb->pitch;
	rotfb_saved_state.screen = fb->screen;
	rotfb_saved_state.bpp = fb->bpp;
	rotfb_saved_state.depth = fb->depth;
	rotfb_saved_state.physical_fb = fb->physical.fb;
	rotfb_saved_state.rotfb_physical_fb = rotfb_info.physical_fb;
	rotfb_saved_state.rotfb_logical_fb = rotfb_info.logical_fb;
	rotfb_saved_state.valid = B_TRUE;

	rotfb_runtime_mapping = mapping;
	rotfb_runtime_mapping_size = map_size;
	rotfb_runtime_shadow = shadow;
	rotfb_runtime_shadow_size = shadow_size;
	rotfb_runtime_owner = fb;

	rotfb_info.physical_fb = mapping;
	rotfb_info.logical_fb = shadow;
	rotfb_info.runtime_ready = B_TRUE;

	fb->physical.fb = mapping;
	fb->fb = shadow;
	fb->shadow_fb = shadow;
	fb->fb_size = shadow_size;
	fb->pitch = rotfb_info.logical_pitch;
	fb->screen.x = rotfb_info.logical_width;
	fb->screen.y = rotfb_info.logical_height;
	fb->bpp = ROTFB_BYTES_PER_PIXEL;
	fb->depth = 32;

	/* Publish the imported boot image before TEM starts using polled I/O. */
	rotfb_runtime_flush_all(fb);

	return (DDI_SUCCESS);
}

void
rotfb_runtime_flush_rect(fb_info_t *fb, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height)
{
	uint32_t xx, yy, xend, yend;

	if (fb == NULL || !rotfb_info.runtime_ready ||
	    rotfb_runtime_mapping == NULL || rotfb_runtime_shadow == NULL ||
	    width == 0 || height == 0 || x >= rotfb_info.logical_width ||
	    y >= rotfb_info.logical_height)
		return;

	if (width > rotfb_info.logical_width - x)
		width = rotfb_info.logical_width - x;
	if (height > rotfb_info.logical_height - y)
		height = rotfb_info.logical_height - y;

	xend = x + width;
	yend = y + height;

	for (yy = y; yy < yend; yy++) {
		for (xx = x; xx < xend; xx++) {
			uint32_t px = rotfb_info.physical_width - 1 - yy;
			uint32_t py = xx;
			uint8_t *src = rotfb_runtime_shadow +
			    (uint64_t)yy * rotfb_info.logical_pitch +
			    (uint64_t)xx * ROTFB_BYTES_PER_PIXEL;
			uint8_t *dst = rotfb_runtime_mapping +
			    (uint64_t)py * rotfb_info.physical_pitch +
			    (uint64_t)px * ROTFB_BYTES_PER_PIXEL;

			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
		}
	}
}

void
rotfb_runtime_flush_all(fb_info_t *fb)
{
	if (fb != NULL && rotfb_info.runtime_ready)
		rotfb_runtime_flush_rect(fb, 0, 0, rotfb_info.logical_width,
		    rotfb_info.logical_height);
}

void
rotfb_runtime_fini(fb_info_t *fb)
{
	uint8_t *mapping, *shadow;
	size_t map_size, shadow_size;

	if (fb == NULL || !rotfb_info.runtime_ready ||
	    rotfb_runtime_owner != fb || !rotfb_saved_state.valid)
		return;

	/*
	 * Restore all externally visible pointers and geometry before freeing
	 * runtime resources.  The boot shadow remains owned by the boot
	 * allocator and is the safe fallback surface after detach.
	 */
	rotfb_info.runtime_ready = B_FALSE;

	fb->fb = rotfb_saved_state.fb;
	fb->shadow_fb = rotfb_saved_state.shadow_fb;
	fb->fb_size = rotfb_saved_state.fb_size;
	fb->pitch = rotfb_saved_state.pitch;
	fb->screen = rotfb_saved_state.screen;
	fb->bpp = rotfb_saved_state.bpp;
	fb->depth = rotfb_saved_state.depth;
	fb->physical.fb = rotfb_saved_state.physical_fb;

	rotfb_info.physical_fb = rotfb_saved_state.rotfb_physical_fb;
	rotfb_info.logical_fb = rotfb_saved_state.rotfb_logical_fb;

	mapping = rotfb_runtime_mapping;
	map_size = rotfb_runtime_mapping_size;
	shadow = rotfb_runtime_shadow;
	shadow_size = rotfb_runtime_shadow_size;

	rotfb_runtime_mapping = NULL;
	rotfb_runtime_mapping_size = 0;
	rotfb_runtime_shadow = NULL;
	rotfb_runtime_shadow_size = 0;
	rotfb_runtime_owner = NULL;
	(void) memset(&rotfb_saved_state, 0, sizeof (rotfb_saved_state));

	if (mapping != NULL)
		gfxp_unmap_kernel_space((gfxp_kva_t)mapping, map_size);
	if (shadow != NULL)
		kmem_free(shadow, shadow_size);
}
