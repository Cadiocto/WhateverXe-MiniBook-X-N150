/*
 * Host-side contract model for the DMA map ownership transitions in
 * iwm_bus.c. This is deliberately NOT a test of illumos DDI or kernel DMA.
 * It checks that the fail-closed state machine expected by patch 0014 keeps
 * ownership on failed unbind and refuses destroy until unbind succeeds.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>

struct dma_map {
	int bound;
	int destroyed;
	unsigned nsegs;
	unsigned long mapsize;
};

static int
model_unload(struct dma_map *m, int ddi_result)
{
	if (m == NULL)
		return (EINVAL);
	if (!m->bound)
		return (0);
	if (ddi_result != 0)
		return (EIO);
	m->bound = 0;
	m->nsegs = 0;
	m->mapsize = 0;
	return (0);
}

static int
model_destroy(struct dma_map *m, int ddi_result)
{
	int err;

	if (m == NULL)
		return (0);
	if (m->bound && (err = model_unload(m, ddi_result)) != 0)
		return (EBUSY);
	m->destroyed = 1;
	return (0);
}

static int
model_too_many_cookies(struct dma_map *m, unsigned long len,
    int unbind_result)
{
	m->bound = 1;              /* ddi_dma_addr_bind_handle succeeded */
	m->mapsize = len;
	m->nsegs = 0;
	if (unbind_result != 0)
		return (EIO);       /* ownership retained for retry */
	m->bound = 0;
	m->mapsize = 0;
	return (EFBIG);
}

int
main(void)
{
	struct dma_map m;

	/* Unbind failure preserves mapping ownership and metadata. */
	m = (struct dma_map){ 1, 0, 1, 4096 };
	assert(model_unload(&m, -1) == EIO);
	assert(m.bound == 1 && m.nsegs == 1 && m.mapsize == 4096);
	puts("DMA_UNBIND_FAILURE_OWNERSHIP=PASS");

	/* Destroy is forbidden while the mapping remains active. */
	assert(model_destroy(&m, -1) == EBUSY);
	assert(m.bound == 1 && m.destroyed == 0);
	puts("DMA_DESTROY_ACTIVE_MAPPING_NEGCTRL=PASS");

	/* A successful retry is the only path that clears ownership. */
	assert(model_destroy(&m, 0) == 0);
	assert(m.bound == 0 && m.nsegs == 0 && m.mapsize == 0);
	assert(m.destroyed == 1);
	puts("DMA_UNBIND_RETRY_THEN_DESTROY=PASS");

	/* Too-many-cookie rollback must retain a failed unbind. */
	m = (struct dma_map){ 0, 0, 0, 0 };
	assert(model_too_many_cookies(&m, 8192, -1) == EIO);
	assert(m.bound == 1 && m.mapsize == 8192 && m.destroyed == 0);
	puts("DMA_BIND_ROLLBACK_FAILURE_OWNERSHIP=PASS");

	/* Successful rollback leaves the map reusable. */
	assert(model_too_many_cookies(&m, 8192, 0) == EFBIG);
	assert(m.bound == 0 && m.mapsize == 0);
	assert(model_destroy(&m, 0) == 0 && m.destroyed == 1);
	puts("DMA_BIND_ROLLBACK_SUCCESS=PASS");

	/* Repeated teardown is idempotent after successful destroy in the model. */
	assert(model_destroy(NULL, -1) == 0);
	puts("DMA_REPEAT_NULL_TEARDOWN=PASS");
	puts("HOST_MODEL_ONLY=YES; ILLUMOS_DDI_BEHAVIOR_NOT_PROVEN=YES");
	return (0);
}
