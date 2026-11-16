#ifndef _SYS_ROTFB_H
#define _SYS_ROTFB_H
#include <sys/types.h>
#include <sys/framebuffer.h>
#include <sys/bootops.h>
#ifdef __cplusplus
extern "C" {
#endif
#define ROTFB_PHYS_WIDTH 1200
#define ROTFB_PHYS_HEIGHT 1920
#define ROTFB_LOGICAL_WIDTH 1920
#define ROTFB_LOGICAL_HEIGHT 1200
#define ROTFB_BYTES_PER_PIXEL 4
typedef struct rotfb_info {
 uint64_t physical_paddr;
 uint8_t *physical_fb;
 uint64_t physical_size;
 uint32_t physical_width, physical_height, physical_pitch, physical_bpp;
 uint8_t *logical_fb;
 uint64_t logical_size;
 uint32_t logical_width, logical_height, logical_pitch;
 boolean_t configured, boot_ready, runtime_ready;
} rotfb_info_t;
extern rotfb_info_t rotfb_info;
int rotfb_boot_configure(fb_info_t *);
int rotfb_boot_shadow_init(bootops_t *, fb_info_t *);
int rotfb_boot_init(fb_info_t *, uint8_t *);
int rotfb_boot_import(fb_info_t *);
void rotfb_boot_flush_rect(fb_info_t *, uint32_t, uint32_t, uint32_t, uint32_t);
void rotfb_boot_flush_all(fb_info_t *);
int rotfb_runtime_init(fb_info_t *);
void rotfb_runtime_fini(fb_info_t *);
void rotfb_runtime_flush_rect(fb_info_t *, uint32_t, uint32_t, uint32_t, uint32_t);
void rotfb_runtime_flush_all(fb_info_t *);
#ifdef __cplusplus
}
#endif
#endif
