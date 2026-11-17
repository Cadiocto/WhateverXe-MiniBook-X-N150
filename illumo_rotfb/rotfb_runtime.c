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
static boolean_t
rotfb_runtime_valid(void)
{
 return (rotfb_info.configured &&
     rotfb_info.physical_width == ROTFB_PHYS_WIDTH &&
     rotfb_info.physical_height == ROTFB_PHYS_HEIGHT &&
     rotfb_info.physical_bpp == ROTFB_BYTES_PER_PIXEL &&
     rotfb_info.physical_pitch >=
     rotfb_info.physical_width * ROTFB_BYTES_PER_PIXEL);
}
int
rotfb_runtime_init(fb_info_t *fb)
{
 uint8_t *mapping, *shadow;
 size_t map_size, shadow_size;
 if (fb == NULL || !rotfb_runtime_valid() ||
     rotfb_info.physical_paddr == 0 || rotfb_info.physical_size == 0)
  return (DDI_FAILURE);
 map_size = (size_t)rotfb_info.physical_size;
 shadow_size = (size_t)rotfb_info.logical_size;
 mapping = (uint8_t *)gfxp_map_kernel_space(rotfb_info.physical_paddr,
     map_size, GFXP_MEMORY_WRITECOMBINED);
 if (mapping == NULL)
  return (DDI_FAILURE);
 shadow = kmem_zalloc(shadow_size, KM_SLEEP);
 if (shadow == NULL) {
  gfxp_unmap_kernel_space((gfxp_kva_t)mapping, map_size);
  return (DDI_FAILURE);
 }
 if (fb->fb != NULL && fb->fb != mapping) {
  size_t copy_size = MIN((size_t)fb->fb_size, shadow_size);
  bcopy(fb->fb, shadow, copy_size);
 }
 rotfb_runtime_mapping = mapping;
 rotfb_runtime_mapping_size = map_size;
 rotfb_runtime_shadow = shadow;
 rotfb_runtime_shadow_size = shadow_size;
 rotfb_info.physical_fb = mapping;
 rotfb_info.logical_fb = shadow;
 rotfb_info.runtime_ready = B_TRUE;
 fb->fb = shadow;
 fb->shadow_fb = shadow;
 fb->fb_size = shadow_size;
 fb->pitch = rotfb_info.logical_pitch;
 fb->screen.x = rotfb_info.logical_width;
 fb->screen.y = rotfb_info.logical_height;
 fb->bpp = ROTFB_BYTES_PER_PIXEL;
 fb->depth = 32;
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
 xend = MIN(x + width, rotfb_info.logical_width);
 yend = MIN(y + height, rotfb_info.logical_height);
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
   dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
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
 if (rotfb_runtime_mapping != NULL) {
  gfxp_unmap_kernel_space((gfxp_kva_t)rotfb_runtime_mapping,
      rotfb_runtime_mapping_size);
  rotfb_runtime_mapping = NULL;
  rotfb_runtime_mapping_size = 0;
 }
 if (rotfb_runtime_shadow != NULL) {
  kmem_free(rotfb_runtime_shadow, rotfb_runtime_shadow_size);
  rotfb_runtime_shadow = NULL;
  rotfb_runtime_shadow_size = 0;
 }
 if (fb != NULL) {
  fb->fb = NULL;
  fb->shadow_fb = NULL;
 }
 rotfb_info.physical_fb = NULL;
 rotfb_info.logical_fb = NULL;
 rotfb_info.runtime_ready = B_FALSE;
}
