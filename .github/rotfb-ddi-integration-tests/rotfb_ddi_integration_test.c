#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DDI_SUCCESS 0
#define DDI_FAILURE (-1)
#define DDI_DETACH 1
#define DDI_SUSPEND 2
#define DDI_DEV_T_ANY 0
#define GFXP_BITMAP 1
#define GFXP_VGATEXT 2
#define CE_WARN 1
#define __unused __attribute__((unused))

typedef struct dev_info {
	int unused;
} dev_info_t;
typedef int ddi_detach_cmd_t;
typedef void *gfxp_kva_t;
typedef struct kmutex {
	int destroyed;
} kmutex_t;

typedef struct fb_info {
	uint8_t *fb;
	uint8_t *shadow_fb;
	uint64_t fb_size;
} fb_info_t;

typedef union gfx_console {
	fb_info_t fb;
} union_gfx_console_t;

typedef struct gfxp_fb_softc {
	dev_info_t *devi;
	int fb_type;
	kmutex_t lock;
	union_gfx_console_t *console;
} gfxp_fb_softc_t;
typedef gfxp_fb_softc_t *gfxp_fb_softc_ptr_t;

typedef struct rotfb_info {
	int configured;
	int runtime_ready;
} rotfb_info_t;

struct vgatext_softc {
	gfxp_fb_softc_ptr_t gfxp_state;
	dev_info_t *devi;
};

extern int gfxp_bm_detach(dev_info_t *, gfxp_fb_softc_ptr_t);
extern int gfxp_fb_detach(dev_info_t *, ddi_detach_cmd_t,
    gfxp_fb_softc_ptr_t);
extern void gfxp_fb_softc_free(gfxp_fb_softc_ptr_t);
extern int gfxp_fb_suspend(gfxp_fb_softc_ptr_t);
extern int gfxp_vga_detach(dev_info_t *, gfxp_fb_softc_ptr_t);
extern int rotfb_runtime_fini(fb_info_t *);
extern int ddi_prop_remove(int, dev_info_t *, const char *);
extern void mutex_destroy(kmutex_t *);
extern void gfxp_unmap_kernel_space(gfxp_kva_t, size_t);
extern void kmem_free(void *, size_t);
extern int ddi_get_instance(dev_info_t *);
extern struct vgatext_softc *getsoftc(int);
extern void ddi_remove_minor_node(dev_info_t *, const char *);
extern int ddi_soft_state_free(void *, int);
extern void cmn_err(int, const char *, ...);

static fb_info_t fb_info;
static rotfb_info_t rotfb_info;
static uint8_t fb_memory[4096];
static uint8_t shadow_memory[4096];
static struct vgatext_softc test_outer;
static gfxp_fb_softc_t test_inner;
static union_gfx_console_t test_console;
static dev_info_t test_device;
static void *vgatext_softc_head;
static int vgatext_force_suspend;
static int injected_fini_result;
static int fini_calls;
static int inner_free_calls;
static int outer_free_calls;
static int minor_remove_calls;
static int property_remove_calls;
static int mutex_destroy_calls;
static int unmap_calls;
static int kmem_free_calls;

#include "production_ddi_entrypoints.inc"

int
rotfb_runtime_fini(fb_info_t *fb)
{
	(void) fb;
	fini_calls++;
	if (injected_fini_result == DDI_SUCCESS)
		rotfb_info.runtime_ready = 0;
	return (injected_fini_result);
}

int
ddi_prop_remove(int any, dev_info_t *devi, const char *name)
{
	(void) any;
	(void) devi;
	(void) name;
	property_remove_calls++;
	return (DDI_SUCCESS);
}

void
mutex_destroy(kmutex_t *lock)
{
	lock->destroyed = 1;
	mutex_destroy_calls++;
}

void
gfxp_unmap_kernel_space(gfxp_kva_t addr, size_t size)
{
	(void) addr;
	(void) size;
	unmap_calls++;
}

void
kmem_free(void *addr, size_t size)
{
	(void) addr;
	(void) size;
	kmem_free_calls++;
}

int
ddi_get_instance(dev_info_t *devi)
{
	(void) devi;
	return (0);
}

struct vgatext_softc *
getsoftc(int instance)
{
	(void) instance;
	return (&test_outer);
}

void
gfxp_fb_softc_free(gfxp_fb_softc_ptr_t ptr)
{
	(void) ptr;
	inner_free_calls++;
}

void
ddi_remove_minor_node(dev_info_t *devi, const char *name)
{
	(void) devi;
	(void) name;
	minor_remove_calls++;
}

int
ddi_soft_state_free(void *head, int instance)
{
	(void) head;
	(void) instance;
	outer_free_calls++;
	return (DDI_SUCCESS);
}

int
gfxp_fb_suspend(gfxp_fb_softc_ptr_t ptr)
{
	(void) ptr;
	return (DDI_SUCCESS);
}

int
gfxp_vga_detach(dev_info_t *devi, gfxp_fb_softc_ptr_t ptr)
{
	(void) devi;
	(void) ptr;
	return (DDI_SUCCESS);
}

void
cmn_err(int level, const char *format, ...)
{
	(void) level;
	(void) format;
}

static void
reset_case(int fini_result)
{
	(void) memset(&fb_info, 0, sizeof (fb_info));
	(void) memset(&rotfb_info, 0, sizeof (rotfb_info));
	(void) memset(&test_outer, 0, sizeof (test_outer));
	(void) memset(&test_inner, 0, sizeof (test_inner));
	(void) memset(&test_console, 0, sizeof (test_console));
	(void) memset(&test_device, 0, sizeof (test_device));
	rotfb_info.configured = 1;
	rotfb_info.runtime_ready = 1;
	test_console.fb.fb = fb_memory;
	test_console.fb.shadow_fb = shadow_memory;
	test_console.fb.fb_size = sizeof (fb_memory);
	test_inner.fb_type = GFXP_BITMAP;
	test_inner.console = &test_console;
	test_outer.gfxp_state = &test_inner;
	injected_fini_result = fini_result;
	fini_calls = 0;
	inner_free_calls = 0;
	outer_free_calls = 0;
	minor_remove_calls = 0;
	property_remove_calls = 0;
	mutex_destroy_calls = 0;
	unmap_calls = 0;
	kmem_free_calls = 0;
}

int
main(void)
{
	int detach_rc;
	uint8_t *saved_fb;
	uint8_t *saved_shadow;

	/*
	 * Scenario A: a failed runtime FINI must leave the complete vgatext
	 * instance intact so that DDI_DETACH can be retried.
	 */
	reset_case(DDI_FAILURE);
	saved_fb = test_console.fb.fb;
	saved_shadow = test_console.fb.shadow_fb;
	detach_rc = vgatext_detach(&test_device, DDI_DETACH);
	if (detach_rc != DDI_FAILURE || fini_calls != 1 ||
	    inner_free_calls != 0 || outer_free_calls != 0 ||
	    minor_remove_calls != 0 || property_remove_calls != 0 ||
	    mutex_destroy_calls != 0 || test_inner.lock.destroyed ||
	    !rotfb_info.runtime_ready ||
	    test_console.fb.fb != saved_fb ||
	    test_console.fb.shadow_fb != saved_shadow ||
	    test_outer.gfxp_state != &test_inner ||
	    test_inner.console != &test_console) {
		(void) fprintf(stderr,
		    "Scenario A: failed detach did not preserve retryable state\n");
		return (1);
	}
	(void) printf("SCENARIO_A_DETACH_FAILURE=PASS\n");

	/*
	 * Scenario B: retry the same instance after the FINI preflight
	 * condition is repaired. Each cleanup operation must occur once.
	 */
	injected_fini_result = DDI_SUCCESS;
	detach_rc = vgatext_detach(&test_device, DDI_DETACH);
	if (detach_rc != DDI_SUCCESS || fini_calls != 2 ||
	    inner_free_calls != 1 || outer_free_calls != 1 ||
	    minor_remove_calls != 1 || property_remove_calls != 1 ||
	    mutex_destroy_calls != 1 || !test_inner.lock.destroyed ||
	    rotfb_info.runtime_ready || unmap_calls != 0 ||
	    kmem_free_calls != 0) {
		(void) fprintf(stderr,
		    "Scenario B: retry cleanup was incomplete or repeated\n");
		return (2);
	}
	(void) printf("SCENARIO_B_DETACH_RETRY=PASS\n");

	/*
	 * Scenario C: an ordinary first-attempt successful detach remains
	 * behaviorally unchanged.
	 */
	reset_case(DDI_SUCCESS);
	detach_rc = vgatext_detach(&test_device, DDI_DETACH);
	if (detach_rc != DDI_SUCCESS || fini_calls != 1 ||
	    inner_free_calls != 1 || outer_free_calls != 1 ||
	    minor_remove_calls != 1 || property_remove_calls != 1 ||
	    mutex_destroy_calls != 1 || !test_inner.lock.destroyed ||
	    rotfb_info.runtime_ready || unmap_calls != 0 ||
	    kmem_free_calls != 0) {
		(void) fprintf(stderr,
		    "Scenario C: ordinary successful detach changed\n");
		return (3);
	}
	(void) printf("SCENARIO_C_NORMAL_DETACH=PASS\n");
	(void) printf("ROTFB_DDI_DETACH_INTEGRATION_HARNESS=PASS\n");
	return (0);
}
