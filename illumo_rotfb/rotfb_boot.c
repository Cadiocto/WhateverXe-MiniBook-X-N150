#include <sys/types.h>
#include <sys/systm.h>
#include <sys/framebuffer.h>
#include <sys/bootconf.h>
#include <sys/rotfb.h>

rotfb_info_t rotfb_info;

static boolean_t
rotfb_boot_valid(void)
{
	uint64_t required_size;

	if (!rotfb_info.configured || rotfb_info.physical_fb == NULL ||
	    rotfb_info.logical_fb == NULL ||
	    rotfb_info.physical_bpp != ROTFB_BYTES_PER_PIXEL ||
	    rotfb_info.physical_width != ROTFB_PHYS_WIDTH ||
	    rotfb_info.physical_height != ROTFB_PHYS_HEIGHT ||
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
rotfb_boot_configure(fb_info_t *fb)
{
	uint64_t row_size, physical_size, logical_size;

	(void) memset(&rotfb_info, 0, sizeof (rotfb_info));

	if (fb == NULL || fb->fb_type != FB_TYPE_RGB ||
	    fb->paddr == 0 || fb->bpp != ROTFB_BYTES_PER_PIXEL ||
	    fb->depth != 32 || fb->screen.x != ROTFB_PHYS_WIDTH ||
	    fb->screen.y != ROTFB_PHYS_HEIGHT || !fb->physical.valid ||
	    fb->physical.paddr != fb->paddr ||
	    fb->physical.width != fb->screen.x ||
	    fb->physical.height != fb->screen.y ||
	    fb->physical.pitch != fb->pitch ||
	    fb->physical.bpp != fb->bpp ||
	    fb->rgb.red.size != 8 || fb->rgb.red.pos != 16 ||
	    fb->rgb.green.size != 8 || fb->rgb.green.pos != 8 ||
	    fb->rgb.blue.size != 8 || fb->rgb.blue.pos != 0)
		return (-1);

	row_size = (uint64_t)fb->screen.x * ROTFB_BYTES_PER_PIXEL;
	if ((uint64_t)fb->pitch < row_size ||
	    (fb->pitch % ROTFB_BYTES_PER_PIXEL) != 0)
		return (-1);

	if (fb->screen.y == 0 ||
	    (uint64_t)fb->pitch > (uint64_t)-1 / fb->screen.y)
		return (-1);

	physical_size = (uint64_t)fb->pitch * fb->screen.y;
	if ((uint64_t)(size_t)physical_size != physical_size ||
	    fb->paddr > (uint64_t)-1 - physical_size ||
	    fb->physical.size != physical_size)
		return (-1);

	rotfb_info.physical_paddr = fb->paddr;
	rotfb_info.physical_width = fb->screen.x;
	rotfb_info.physical_height = fb->screen.y;
	rotfb_info.physical_pitch = fb->pitch;
	rotfb_info.physical_bpp = fb->bpp;
	rotfb_info.physical_size = physical_size;
	rotfb_info.logical_width = ROTFB_LOGICAL_WIDTH;
	rotfb_info.logical_height = ROTFB_LOGICAL_HEIGHT;
	rotfb_info.logical_pitch =
	    ROTFB_LOGICAL_WIDTH * ROTFB_BYTES_PER_PIXEL;

	logical_size = (uint64_t)rotfb_info.logical_pitch *
	    rotfb_info.logical_height;
	if ((uint64_t)(size_t)logical_size != logical_size)
		return (-1);

	rotfb_info.logical_size = logical_size;
	rotfb_info.configured = B_TRUE;
	return (0);
}

int
rotfb_boot_shadow_init(struct bootops *bops, fb_info_t *fb)
{
	uint8_t *shadow;
	size_t logical_size;

	if (bops == NULL || bops->bsys_alloc == NULL || fb == NULL ||
	    !rotfb_info.configured)
		goto fail;

	if (rotfb_info.logical_fb != NULL) {
		if (fb->fb == rotfb_info.logical_fb)
			return (0);
		goto fail;
	}

	/*
	 * boot_fb_init() has already installed the physical framebuffer
	 * address.  The logical shadow is allocated here, then boot_fb_init()
	 * is called again by boot_fb_shadow_init() to import the physical
	 * image and recalculate terminal geometry for the rotated surface.
	 */
	if (fb->fb == NULL || fb->paddr != rotfb_info.physical_paddr ||
	    fb->screen.x != rotfb_info.physical_width ||
	    fb->screen.y != rotfb_info.physical_height ||
	    fb->pitch != rotfb_info.physical_pitch ||
	    fb->bpp != rotfb_info.physical_bpp ||
	    fb->fb_size < rotfb_info.physical_size)
		goto fail;

	logical_size = (size_t)rotfb_info.logical_size;
	shadow = (uint8_t *)bops->bsys_alloc(NULL, NULL, logical_size,
	    MMU_PAGESIZE);
	if (shadow == NULL)
		goto fail;

	(void) memset(shadow, 0, logical_size);
	rotfb_info.logical_fb = shadow;
	fb->shadow_fb = NULL;
	fb->fb = shadow;
	fb->fb_size = rotfb_info.logical_size;
	fb->pitch = rotfb_info.logical_pitch;
	fb->screen.x = rotfb_info.logical_width;
	fb->screen.y = rotfb_info.logical_height;
	fb->bpp = ROTFB_BYTES_PER_PIXEL;
	fb->depth = 32;

	return (0);

fail:
	rotfb_info.configured = B_FALSE;
	rotfb_info.boot_ready = B_FALSE;
	rotfb_info.runtime_ready = B_FALSE;
	rotfb_info.physical_fb = NULL;
	rotfb_info.logical_fb = NULL;
	return (-1);
}

int
rotfb_boot_init(fb_info_t *fb, uint8_t *physical_fb)
{
	if (fb == NULL || physical_fb == NULL || !rotfb_info.configured ||
	    rotfb_info.logical_fb == NULL ||
	    fb->paddr != rotfb_info.physical_paddr ||
	    !fb->physical.valid ||
	    fb->physical.paddr != rotfb_info.physical_paddr ||
	    fb->physical.width != rotfb_info.physical_width ||
	    fb->physical.height != rotfb_info.physical_height ||
	    fb->physical.pitch != rotfb_info.physical_pitch ||
	    fb->physical.bpp != rotfb_info.physical_bpp ||
	    fb->physical.size != rotfb_info.physical_size)
		return (-1);

	rotfb_info.physical_fb = physical_fb;
	fb->physical.fb = physical_fb;
	/* rotfb_boot_import() requires fb->fb to name the logical destination. */
	fb->fb = rotfb_info.logical_fb;
	rotfb_info.boot_ready = B_TRUE;
	return (0);
}

int
rotfb_boot_import(fb_info_t *fb)
{
	uint32_t x, y;

	if (fb == NULL || !rotfb_boot_valid() || !rotfb_info.boot_ready ||
	    rotfb_info.runtime_ready || fb->fb != rotfb_info.logical_fb ||
	    fb->fb_size < rotfb_info.logical_size ||
	    fb->bpp != ROTFB_BYTES_PER_PIXEL || fb->depth != 32 ||
	    fb->pitch != rotfb_info.logical_pitch ||
	    fb->screen.x != rotfb_info.logical_width ||
	    fb->screen.y != rotfb_info.logical_height)
		return (-1);

	for (y = 0; y < rotfb_info.logical_height; y++) {
		for (x = 0; x < rotfb_info.logical_width; x++) {
			uint32_t px = rotfb_info.physical_width - 1 - y;
			uint32_t py = x;
			uint8_t *src = rotfb_info.physical_fb +
			    (uint64_t)py * rotfb_info.physical_pitch +
			    (uint64_t)px * ROTFB_BYTES_PER_PIXEL;
			uint8_t *dst = rotfb_info.logical_fb +
			    (uint64_t)y * rotfb_info.logical_pitch +
			    (uint64_t)x * ROTFB_BYTES_PER_PIXEL;

			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
		}
	}

	return (0);
}

/*
 * dboot cannot call membar_producer(): that symbol is provided by the
 * kernel atomic objects, which are not linked into dboot. Match the x86
 * producer barrier (SFENCE) locally so prior WC stores reach the display
 * framebuffer before the flush returns.
 */
static void
rotfb_boot_wc_store_barrier(void)
{
	__asm__ __volatile__("sfence" : : : "memory");
}

void
rotfb_boot_flush_rect(fb_info_t *fb, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height)
{
	uint32_t xx, yy, xend, yend;

	if (fb == NULL || !rotfb_boot_valid() || !rotfb_info.boot_ready ||
	    fb->fb != rotfb_info.logical_fb ||
	    fb->pitch != rotfb_info.logical_pitch ||
	    fb->screen.x != rotfb_info.logical_width ||
	    fb->screen.y != rotfb_info.logical_height ||
	    fb->fb_size < rotfb_info.logical_size ||
	    fb->bpp != ROTFB_BYTES_PER_PIXEL || fb->depth != 32 ||
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
			uint8_t *src = rotfb_info.logical_fb +
			    (uint64_t)yy * rotfb_info.logical_pitch +
			    (uint64_t)xx * ROTFB_BYTES_PER_PIXEL;
			uint8_t *dst = rotfb_info.physical_fb +
			    (uint64_t)py * rotfb_info.physical_pitch +
			    (uint64_t)px * ROTFB_BYTES_PER_PIXEL;

			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
		}
	}
	rotfb_boot_wc_store_barrier();
}

void
rotfb_boot_flush_all(fb_info_t *fb)
{
	if (fb != NULL)
		rotfb_boot_flush_rect(fb, 0, 0, rotfb_info.logical_width,
		    rotfb_info.logical_height);
}
