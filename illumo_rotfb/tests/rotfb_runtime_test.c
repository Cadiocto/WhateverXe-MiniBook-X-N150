#define _DEFAULT_SOURCE 1
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "sys/rotfb.h"
#include "sys/gfx_private.h"
#include "sys/kmem.h"
#include "sys/ddi.h"

rotfb_info_t rotfb_info;

static int fail_map;
static int fail_alloc;
static unsigned map_calls;
static unsigned unmap_calls;
static unsigned alloc_calls;
static unsigned free_calls;
static unsigned barrier_calls;
static uint8_t *map_base;
static uint8_t *map_user;
static uint8_t *boot_physical;
static uint8_t *boot_logical;
static const size_t physical_size = 4800u * 1920u;
static const size_t logical_size = 7680u * 1200u;

uint32_t
atomic_add_32_nv(volatile uint32_t *p, int32_t v)
{
	return (__atomic_add_fetch(p, (uint32_t)v, __ATOMIC_SEQ_CST));
}

uint32_t
atomic_cas_32(volatile uint32_t *p, uint32_t oldv, uint32_t newv)
{
	uint32_t expected = oldv;
	(void) __atomic_compare_exchange_n(p, &expected, newv, 0,
	    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return (expected);
}

uint32_t
atomic_dec_32(volatile uint32_t *p)
{
	return (__atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST));
}

uint32_t
atomic_swap_32(volatile uint32_t *p, uint32_t v)
{
	return (__atomic_exchange_n(p, v, __ATOMIC_SEQ_CST));
}

void
membar_producer(void)
{
	atomic_thread_fence(memory_order_release);
	barrier_calls++;
}

void
delay(int ticks)
{
	(void) ticks;
	(void) sched_yield();
}

void *
gfxp_map_kernel_space(uint64_t paddr, size_t size, int attr)
{
	map_calls++;
	assert(paddr == UINT64_C(0x4000000000));
	assert(size == physical_size);
	assert(attr == GFXP_MEMORY_WRITECOMBINED);
	if (fail_map)
		return (NULL);
	map_base = malloc(size + 32);
	assert(map_base != NULL);
	memset(map_base, 0xA7, size + 32);
	map_user = map_base + 16;
	return (map_user);
}

void
gfxp_unmap_kernel_space(gfxp_kva_t kva, size_t size)
{
	uint8_t *p = kva;
	assert(p == map_user);
	assert(size == physical_size);
	for (unsigned i = 0; i < 16; i++) {
		assert(map_base[i] == 0xA7);
		assert(map_user[size + i] == 0xA7);
	}
	free(map_base);
	map_base = map_user = NULL;
	unmap_calls++;
}

void *
kmem_zalloc(size_t size, int flag)
{
	alloc_calls++;
	assert(size == logical_size);
	assert(flag == KM_NOSLEEP);
	if (fail_alloc)
		return (NULL);
	return (calloc(1, size));
}

void
kmem_free(void *p, size_t size)
{
	assert(p != NULL);
	assert(size == logical_size);
	free(p);
	free_calls++;
}

static void
set_pixel(uint8_t *p, uint32_t x, uint32_t y)
{
	size_t o = (size_t)y * 7680u + (size_t)x * 4u;
	p[o + 0] = (uint8_t)(x & 0xff);
	p[o + 1] = (uint8_t)(y & 0xff);
	p[o + 2] = (uint8_t)((x ^ y) & 0xff);
	p[o + 3] = 0x5a;
}

static void
init_fixture(fb_info_t *fb)
{
	memset(&rotfb_info, 0, sizeof (rotfb_info));
	memset(fb, 0, sizeof (*fb));
	boot_physical = malloc(physical_size);
	boot_logical = malloc(logical_size);
	assert(boot_physical != NULL && boot_logical != NULL);
	memset(boot_physical, 0x3c, physical_size);
	for (uint32_t y = 0; y < 1200; y++)
		for (uint32_t x = 0; x < 1920; x++)
			set_pixel(boot_logical, x, y);

	rotfb_info.physical_paddr = UINT64_C(0x4000000000);
	rotfb_info.physical_fb = boot_physical;
	rotfb_info.physical_size = physical_size;
	rotfb_info.physical_width = 1200;
	rotfb_info.physical_height = 1920;
	rotfb_info.physical_pitch = 4800;
	rotfb_info.physical_bpp = 4;
	rotfb_info.logical_fb = boot_logical;
	rotfb_info.logical_size = logical_size;
	rotfb_info.logical_width = 1920;
	rotfb_info.logical_height = 1200;
	rotfb_info.logical_pitch = 7680;
	rotfb_info.configured = B_TRUE;
	rotfb_info.boot_ready = B_TRUE;

	fb->fb = boot_logical;
	fb->shadow_fb = NULL;
	fb->fb_size = logical_size;
	fb->pitch = 7680;
	fb->screen.x = 1920;
	fb->screen.y = 1200;
	fb->bpp = 4;
	fb->depth = 32;
	fb->physical.valid = B_TRUE;
	fb->physical.paddr = rotfb_info.physical_paddr;
	fb->physical.width = 1200;
	fb->physical.height = 1920;
	fb->physical.pitch = 4800;
	fb->physical.bpp = 4;
	fb->physical.size = physical_size;
	fb->physical.fb = boot_physical;
}

static void
check_initial_rotation(const uint8_t *physical, const uint8_t *logical)
{
	for (uint32_t y = 0; y < 1200; y++) {
		for (uint32_t x = 0; x < 1920; x++) {
			uint32_t px = 1199u - y;
			uint32_t py = x;
			const uint8_t *s = logical + (size_t)y * 7680u + x * 4u;
			const uint8_t *d = physical + (size_t)py * 4800u + px * 4u;
			if (memcmp(s, d, 4) != 0) {
				fprintf(stderr, "pixel mismatch logical=(%u,%u) physical=(%u,%u)\n", x, y, px, py);
				exit(2);
			}
		}
	}
}

static void
check_clipped_flush(uint8_t *physical, uint8_t *shadow)
{
	memset(physical, 0xCC, physical_size);
	memset(shadow, 0x11, logical_size);
	for (uint32_t y = 1198; y < 1200; y++) {
		for (uint32_t x = 1918; x < 1920; x++) {
			size_t o = (size_t)y * 7680u + (size_t)x * 4u;
			shadow[o + 0] = (uint8_t)(x & 0xff);
			shadow[o + 1] = (uint8_t)(y & 0xff);
			shadow[o + 2] = 0xE1;
			shadow[o + 3] = 0xE2;
		}
	}
	rotfb_runtime_flush_rect(NULL, 1918, 1198, UINT32_MAX, UINT32_MAX);
	/* NULL must be a no-op. */
	for (size_t i = 0; i < physical_size; i++)
		assert(physical[i] == 0xCC);
	/* The valid owner clips UINT32_MAX dimensions to the final 2x2 pixels. */
	/* The caller supplies fb below through the global test fixture. */
}

static fb_info_t *global_fb;
static void
flush_last_two_by_two(void)
{
	uint8_t *physical = map_user;
	uint8_t *shadow = global_fb->fb;
	rotfb_runtime_flush_rect(global_fb, 1918, 1198, UINT32_MAX, UINT32_MAX);
	for (uint32_t y = 0; y < 1200; y++) {
		for (uint32_t x = 0; x < 1920; x++) {
			uint32_t px = 1199u - y;
			uint32_t py = x;
			const uint8_t *d = physical + (size_t)py * 4800u + px * 4u;
			if (x >= 1918 && y >= 1198) {
				const uint8_t *s = shadow + (size_t)y * 7680u + x * 4u;
				assert(memcmp(d, s, 4) == 0);
			} else {
				assert(d[0] == 0xCC && d[1] == 0xCC &&
				    d[2] == 0xCC && d[3] == 0xCC);
			}
		}
	}
	/* Out-of-range origin is rejected without touching the physical surface. */
	memset(physical, 0xD3, physical_size);
	rotfb_runtime_flush_rect(global_fb, UINT32_MAX, 0, 1, 1);
	for (size_t i = 0; i < physical_size; i++)
		assert(physical[i] == 0xD3);
}

typedef struct fini_thread_arg {
	fb_info_t *fb;
	_Atomic int done;
	int result;
} fini_thread_arg_t;

static void *
fini_thread(void *arg)
{
	fini_thread_arg_t *a = arg;
	a->result = rotfb_runtime_fini(a->fb);
	atomic_store_explicit(&a->done, 1, memory_order_release);
	return (NULL);
}

int
main(void)
{
	fb_info_t fb;
	uint8_t *boot_fb;
	uint8_t *runtime_fb;
	uint8_t fake;
	int entered;
	pthread_t thread;
	fini_thread_arg_t arg;
	int gate_closed = 0;

	init_fixture(&fb);
	global_fb = &fb;
	boot_fb = fb.fb;

	/* The unmanaged pre-runtime callback path remains transparent. */
	assert(rotfb_runtime_callback_enter() == 0);

	/* Invalid production geometry must be rejected before mapping. */
	{
		uint32_t old_pitch = rotfb_info.physical_pitch;
		rotfb_info.physical_pitch = 4796;
		fb.physical.pitch = 4796;
		assert(rotfb_runtime_init(&fb) == DDI_FAILURE);
		assert(map_calls == 0 && fb.fb == boot_fb);
		rotfb_info.physical_pitch = old_pitch;
		fb.physical.pitch = old_pitch;
	}
	{
		uint64_t old_size = rotfb_info.physical_size;
		rotfb_info.physical_size = physical_size - 1;
		fb.physical.size = physical_size - 1;
		assert(rotfb_runtime_init(&fb) == DDI_FAILURE);
		assert(map_calls == 0 && fb.fb == boot_fb);
		rotfb_info.physical_size = old_size;
		fb.physical.size = old_size;
	}
	{
		uint32_t old_pitch = rotfb_info.logical_pitch;
		rotfb_info.logical_pitch = 7676;
		fb.pitch = 7676;
		assert(rotfb_runtime_init(&fb) == DDI_FAILURE);
		assert(map_calls == 0 && fb.fb == boot_fb);
		rotfb_info.logical_pitch = old_pitch;
		fb.pitch = old_pitch;
	}

	/* Mapping failure must leave every published pointer untouched. */
	fail_map = 1;
	assert(rotfb_runtime_init(&fb) == DDI_FAILURE);
	assert(fb.fb == boot_fb && fb.shadow_fb == NULL);
	assert(fb.physical.fb == boot_physical && !rotfb_info.runtime_ready);
	assert(unmap_calls == 0 && alloc_calls == 0);

	/* Allocation failure must unmap the WC mapping and publish no state. */
	fail_map = 0;
	fail_alloc = 1;
	assert(rotfb_runtime_init(&fb) == DDI_FAILURE);
	assert(fb.fb == boot_fb && fb.shadow_fb == NULL);
	assert(fb.physical.fb == boot_physical && !rotfb_info.runtime_ready);
	assert(unmap_calls == 1 && free_calls == 0);

	/* Successful init imports the boot image and performs the full CW flush. */
	fail_alloc = 0;
	assert(rotfb_runtime_init(&fb) == DDI_SUCCESS);
	assert(rotfb_info.runtime_ready && fb.fb != boot_fb);
	assert(fb.fb == fb.shadow_fb && fb.physical.fb == map_user);
	runtime_fb = fb.fb;
	assert(memcmp(runtime_fb, boot_fb, logical_size) == 0);
	check_initial_rotation(map_user, runtime_fb);
	assert(barrier_calls >= 1);
	entered = rotfb_runtime_callback_enter();
	assert(entered == 1);
	rotfb_runtime_callback_exit(entered);

	/* Invalid pointer state must fail rollback without freeing live resources. */
	fb.fb = &fake;
	assert(rotfb_runtime_fini(&fb) == DDI_FAILURE);
	assert(rotfb_info.runtime_ready && unmap_calls == 1 && free_calls == 0);
	entered = rotfb_runtime_callback_enter();
	assert(entered == 1); /* rollback failure leaves the gate open */
	rotfb_runtime_callback_exit(entered);
	fb.fb = runtime_fb;

	/* Clipping must handle UINT32_MAX dimensions without crossing bounds. */
	check_clipped_flush(map_user, runtime_fb);
	flush_last_two_by_two();

	/* Fini must copy the final logical image back to the boot-owned shadow. */
	uint8_t *expected_boot_image = malloc(logical_size);
	assert(expected_boot_image != NULL);
	memcpy(expected_boot_image, runtime_fb, logical_size);

	/* Hold a callback while fini closes the gate; fini must wait for its exit. */
	entered = rotfb_runtime_callback_enter();
	assert(entered == 1);
	memset(&arg, 0, sizeof (arg));
	arg.fb = &fb;
	atomic_init(&arg.done, 0);
	assert(pthread_create(&thread, NULL, fini_thread, &arg) == 0);
	for (unsigned i = 0; i < 1000000; i++) {
		int rc = rotfb_runtime_callback_enter();
		if (rc < 0) {
			gate_closed = 1;
			break;
		}
		rotfb_runtime_callback_exit(rc);
		(void) sched_yield();
	}
	assert(gate_closed);
	assert(atomic_load_explicit(&arg.done, memory_order_acquire) == 0);
	assert(unmap_calls == 1 && free_calls == 0); /* resources still live */
	rotfb_runtime_callback_exit(entered);
	assert(pthread_join(thread, NULL) == 0);
	assert(arg.result == DDI_SUCCESS);
	assert(atomic_load_explicit(&arg.done, memory_order_acquire) == 1);
	assert(!rotfb_info.runtime_ready && rotfb_info.boot_ready);
	assert(fb.fb == boot_fb && fb.shadow_fb == NULL);
	assert(fb.physical.fb == boot_physical);
	assert(rotfb_info.logical_fb == boot_fb && rotfb_info.physical_fb == boot_physical);
	assert(memcmp(boot_fb, expected_boot_image, logical_size) == 0);
	free(expected_boot_image);
	assert(unmap_calls == 2 && free_calls == 1);
	assert(rotfb_runtime_callback_enter() < 0); /* stale polledio rejected */

	/* A later attach may initialize again and reopen the managed callback gate. */
	assert(rotfb_runtime_init(&fb) == DDI_SUCCESS);
	assert(rotfb_info.runtime_ready && fb.fb == fb.shadow_fb);
	assert(memcmp(fb.fb, boot_fb, logical_size) == 0);
	check_initial_rotation(map_user, fb.fb);
	entered = rotfb_runtime_callback_enter();
	assert(entered == 1);
	rotfb_runtime_callback_exit(entered);
	assert(rotfb_runtime_fini(&fb) == DDI_SUCCESS);
	assert(!rotfb_info.runtime_ready && rotfb_info.boot_ready);
	assert(fb.fb == boot_fb && fb.physical.fb == boot_physical);
	assert(unmap_calls == 3 && free_calls == 2);

	free(boot_physical);
	free(boot_logical);
	puts("ROTFB production runtime C tests: PASS");
	puts("  geometry validation before mapping: PASS");
	puts("  mapping failure: PASS");
	puts("  allocation failure/unmap rollback: PASS");
	puts("  initial sync and CW mapping: all 2,304,000 pixels PASS");
	puts("  clipping/canaries and UINT32_MAX bounds: PASS");
	puts("  pointer mismatch rollback: PASS");
	puts("  callback quiesce/drain: PASS");
	puts("  stale callback rejection: PASS");
	puts("  fini/re-init/teardown: PASS");
	return (0);
}
