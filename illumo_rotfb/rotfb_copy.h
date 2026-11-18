#ifndef _SYS_ROTFB_COPY_H
#define _SYS_ROTFB_COPY_H

/*
 * Copy a rectangle one scanline at a time, preserving memmove's overlap
 * ordering. If framebuffer and shadow destinations alias in text mode,
 * skip the duplicate shadow copy.
 */
typedef void *(*rotfb_copy_memmove_t)(void *, const void *, size_t);

static inline void
rotfb_copy_rows(uint8_t *dst, const uint8_t *src, uint8_t *shadow_dst,
    size_t width, uint32_t height, uint32_t pitch, int copy_fb,
    int copy_shadow, int forward, rotfb_copy_memmove_t move)
{
	uint32_t row;

	if (dst == NULL || src == NULL || shadow_dst == NULL || width == 0 ||
	    height == 0 || pitch == 0 || width > pitch || move == NULL)
		return;

	if (forward) {
		for (row = 0; row < height; row++) {
			uint64_t offset = (uint64_t)row * pitch;

			if (copy_fb)
				(void) move(dst + offset, src + offset, width);
			if (copy_shadow && (!copy_fb || shadow_dst != dst))
				(void) move(shadow_dst + offset, src + offset, width);
		}
	} else {
		for (row = height; row != 0; ) {
			uint64_t offset;

			row--;
			offset = (uint64_t)row * pitch;
			if (copy_fb)
				(void) move(dst + offset, src + offset, width);
			if (copy_shadow && (!copy_fb || shadow_dst != dst))
				(void) move(shadow_dst + offset, src + offset, width);
		}
	}
}

#endif /* _SYS_ROTFB_COPY_H */
