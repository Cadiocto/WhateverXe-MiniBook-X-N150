#include <sys/types.h>
#include <sys/systm.h>
#include <sys/framebuffer.h>
#include <sys/bootconf.h>
#include <sys/rotfb.h>
#include <sys/sysmacros.h>
rotfb_info_t rotfb_info;
static boolean_t
rotfb_boot_valid(void)
{
 return (rotfb_info.configured && rotfb_info.physical_fb != NULL &&
     rotfb_info.logical_fb != NULL &&
     rotfb_info.physical_bpp == ROTFB_BYTES_PER_PIXEL &&
     rotfb_info.physical_width == ROTFB_PHYS_WIDTH &&
     rotfb_info.physical_height == ROTFB_PHYS_HEIGHT);
}
int
rotfb_boot_configure(fb_info_t *fb)
{
 if (fb == NULL || fb->paddr == 0 || fb->pitch == 0 ||
     fb->screen.x == 0 || fb->screen.y == 0 || fb->bpp != 4)
  return (-1);
 bzero(&rotfb_info, sizeof (rotfb_info));
 rotfb_info.physical_paddr = fb->paddr;
 rotfb_info.physical_width = fb->screen.x;
 rotfb_info.physical_height = fb->screen.y;
 rotfb_info.physical_pitch = fb->pitch;
 rotfb_info.physical_bpp = fb->bpp;
 rotfb_info.physical_size = (uint64_t)fb->pitch * fb->screen.y;
 if (rotfb_info.physical_width != ROTFB_PHYS_WIDTH ||
     rotfb_info.physical_height != ROTFB_PHYS_HEIGHT)
  return (-1);
 rotfb_info.logical_width = ROTFB_LOGICAL_WIDTH;
 rotfb_info.logical_height = ROTFB_LOGICAL_HEIGHT;
 rotfb_info.logical_pitch = ROTFB_LOGICAL_WIDTH * ROTFB_BYTES_PER_PIXEL;
 rotfb_info.logical_size = (uint64_t)rotfb_info.logical_pitch *
     ROTFB_LOGICAL_HEIGHT;
 rotfb_info.configured = B_TRUE;
 return (0);
}
int
rotfb_boot_shadow_init(struct bootops *bops, fb_info_t *fb)
{
 uint8_t *shadow;
 if (bops == NULL || fb == NULL || !rotfb_info.configured)
  return (-1);
 shadow = (uint8_t *)bops->bsys_alloc(NULL, NULL,
     rotfb_info.logical_size, MMU_PAGESIZE);
 if (shadow == NULL)
  return (-1);
 bzero(shadow, rotfb_info.logical_size);
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
}
int
rotfb_boot_init(fb_info_t *fb, uint8_t *physical_fb)
{
 if (fb == NULL || physical_fb == NULL || !rotfb_info.configured ||
     rotfb_info.logical_fb == NULL)
  return (-1);
 rotfb_info.physical_fb = physical_fb;
 fb->physical.fb = physical_fb;
 rotfb_info.boot_ready = B_TRUE;
 return (0);
}
int
rotfb_boot_import(fb_info_t *fb)
{
 uint32_t x, y;
 if (fb == NULL || !rotfb_boot_valid() || !rotfb_info.boot_ready)
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
   dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  }
 }
 return (0);
}
void
rotfb_boot_flush_rect(fb_info_t *fb, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height)
{
 uint32_t xx, yy, xend, yend;
 if (fb == NULL || !rotfb_boot_valid() || !rotfb_info.boot_ready ||
     width == 0 || height == 0 || x >= rotfb_info.logical_width ||
     y >= rotfb_info.logical_height)
  return;
 xend = MIN(x + width, rotfb_info.logical_width);
 yend = MIN(y + height, rotfb_info.logical_height);
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
   dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  }
 }
}
void
rotfb_boot_flush_all(fb_info_t *fb)
{
 if (fb != NULL)
  rotfb_boot_flush_rect(fb, 0, 0, rotfb_info.logical_width,
      rotfb_info.logical_height);
}
