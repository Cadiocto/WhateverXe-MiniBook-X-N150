/*
 * Copyright (c) 2026 Mike Aldred
 * Copyright (c) 2001-2002 Matthieu Herrb
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * illumosfb_driver.c - Xorg DDX driver for illumos framebuffer
 *
 * A simple framebuffer-based Xorg driver for illumos systems using the
 * UEFI GOP framebuffer exposed via /dev/fb0.  Modeled after xf86-video-wsfb
 * (NetBSD) and xf86-video-scfb (FreeBSD).
 *
 * Uses FBIOGATTR to query framebuffer parameters, mmap for framebuffer
 * access, ShadowFB for software rendering, and KDSETMODE for console
 * switching.
 */

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/fbio.h>
#include <sys/kd.h>
#include <sys/visual_io.h>

#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Module.h"

#include "mipointer.h"
#include "micmap.h"
#include "colormapst.h"

#include "fb.h"
#include "shadow.h"
#include "damage.h"
#include "regionstr.h"


#define	ILFB_VERSION		1
#define	ILFB_NAME		"illumosfb"
#define	ILFB_DRIVER_NAME	"illumosfb"
#define	ILFB_DEFAULT_DEV	"/dev/fb0"

/* Fixed ROTFB logical and physical geometries. */
#define ILFB_LOGICAL_WIDTH	1920
#define ILFB_LOGICAL_HEIGHT	1200
#define ILFB_PHYSICAL_WIDTH	1200
#define ILFB_PHYSICAL_HEIGHT	1920
#define ILFB_PHYSICAL_BPP	32
#define ILFB_PHYSICAL_PITCH	4800
#define ILFB_PHYSICAL_SIZE	((size_t)ILFB_PHYSICAL_PITCH * \
    (size_t)ILFB_PHYSICAL_HEIGHT)

/* Private data */
typedef struct {
	int			fd;
	unsigned char		*fbmem;
	size_t			fbmem_len;
	int			linebytes;
	int			width;		/* physical width */
	int			height;		/* physical height */
	int			fb_bpp;		/* physical bits per pixel */

	void			*shadow;
	size_t			shadow_len;
	int			shadow_stride;
	Bool			shadowFB;
	shadowUpdateProc	rotate_update;

	Bool			fb_mapped;
	Bool			graphics_mode;
	Bool			vt_active;
	Bool			fb_initialized;
	Bool			shadow_setup;
	Bool			screen_pixmap_ready;
	Bool			shadow_registered;
	Bool			terminal_failure;

	CloseScreenProcPtr	CloseScreen;
	CreateScreenResourcesProcPtr CreateScreenResources;

	EntityInfoPtr		pEnt;
	OptionInfoPtr		Options;
} IlFbRec, *IlFbPtr;

#define	ILFBPTR(p)	((IlFbPtr)((p)->driverPrivate))

/* Options */
typedef enum {
	OPTION_SHADOW_FB,
	OPTION_DEVICE,
} IlFbOpts;

static const OptionInfoRec IlFbOptions[] = {
	{ OPTION_SHADOW_FB,	"ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_DEVICE,	"Device",	OPTV_STRING,	{0}, FALSE },
	{ -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

/* Chipsets */
static SymTabRec IlFbChipsets[] = {
	{ 0, ILFB_NAME },
	{ -1, NULL }
};

/* Forward declarations */
static Bool IlFbProbe(DriverPtr drv, int flags);
static Bool IlFbPreInit(ScrnInfoPtr pScrn, int flags);
static Bool IlFbScreenInit(ScreenPtr pScreen, int argc, char **argv);
static Bool IlFbCloseScreen(ScreenPtr pScreen);
static Bool IlFbFailedCloseScreen(ScreenPtr pScreen);
static void IlFbAbortAfterFbInit(ScreenPtr pScreen, ScrnInfoPtr pScrn,
    IlFbPtr fPtr);
static Bool IlFbEnterVT(ScrnInfoPtr pScrn);
static void IlFbLeaveVT(ScrnInfoPtr pScrn);
static Bool IlFbSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
static ModeStatus IlFbValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode,
    Bool verbose, int flags);
static Bool IlFbSaveScreen(ScreenPtr pScreen, int mode);
static Bool IlFbDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
    void *ptr);

static const OptionInfoRec *
IlFbAvailableOptions(int chipid, int busid)
{
	return (IlFbOptions);
}

static void
IlFbIdentify(int flags)
{
	xf86PrintChipsets(ILFB_NAME,
	    "driver for illumos framebuffer", IlFbChipsets);
}

_X_EXPORT DriverRec ILLUMOSFB = {
	ILFB_VERSION,
	ILFB_DRIVER_NAME,
	IlFbIdentify,
	IlFbProbe,
	IlFbAvailableOptions,
	NULL,
	0,
	IlFbDriverFunc,
	NULL,			/* supported_devices */
	NULL,			/* PciProbe */
	NULL			/* platformProbe */
};

/* Module setup */
static pointer
IlFbSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	if (!setupDone) {
		setupDone = TRUE;
		xf86AddDriver(&ILLUMOSFB, module, HaveDriverFuncs);
		return ((pointer)1);
	} else {
		if (errmaj != NULL)
			*errmaj = LDR_ONCEONLY;
		return (NULL);
	}
}

static XF86ModuleVersionInfo IlFbVersRec = {
	ILFB_DRIVER_NAME,
	"illumos",
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	0, 1, 0,		/* version major.minor.patch */
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData illumosfbModuleData = {
	&IlFbVersRec, IlFbSetup, NULL
};

/*
 * DriverFunc - Tell the server we don't need VGA or PCI I/O.
 */
static Bool
IlFbDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op, void *ptr)
{
	xorgHWFlags *flag;

	switch (op) {
	case GET_REQUIRED_HW_INTERFACES:
		flag = (xorgHWFlags *)ptr;
		(*flag) = 0;
		return (TRUE);
	default:
		return (FALSE);
	}
}

/*
 * Open the framebuffer device, verify it's an illumos_fb, and query attrs.
 * Returns fd on success, -1 on failure.
 */
static int
IlFbOpenDev(ScrnInfoPtr pScrn, const char *dev, IlFbPtr fPtr)
{
	struct vis_identifier ident;
	struct fbgattr attr;
	struct gfxfb_info gfxfb;
	uint64_t min_pitch, required_size;
	uint32_t pitch;
	int fd;

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "open %s: %s\n", dev, strerror(errno));
		return (-1);
	}

	if (ioctl(fd, VIS_GETIDENTIFIER, &ident) != 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "VIS_GETIDENTIFIER failed: %s\n", strerror(errno));
		close(fd);
		return (-1);
	}

	if (strcmp(ident.name, "illumos_fb") != 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Not an illumos framebuffer (got \"%s\")\n",
		    ident.name);
		close(fd);
		return (-1);
	}

	(void) memset(&attr, 0, sizeof (attr));
	if (ioctl(fd, FBIOGATTR, &attr) != 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "FBIOGATTR failed: %s\n", strerror(errno));
		close(fd);
		return (-1);
	}

	/* fbtype fields are signed ints in the illumos fbio ABI. */
	if (attr.fbtype.fb_width <= 0 || attr.fbtype.fb_height <= 0 ||
	    attr.fbtype.fb_depth <= 0 || attr.fbtype.fb_size <= 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Invalid framebuffer metadata: %dx%d, %d bpp, size %d\n",
		    attr.fbtype.fb_width, attr.fbtype.fb_height,
		    attr.fbtype.fb_depth, attr.fbtype.fb_size);
		close(fd);
		return (-1);
	}

	/* dev_specific is an embedded int[8], not a pointer. */
	if ((attr.sattr.flags & FB_ATTR_DEVSPECIFIC) == 0 ||
	    sizeof (attr.sattr.dev_specific) < sizeof (struct gfxfb_info)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Framebuffer device-specific metadata is unavailable\n");
		close(fd);
		return (-1);
	}
	(void)memcpy(&gfxfb, &attr.sattr.dev_specific[0], sizeof (gfxfb));
	pitch = gfxfb.pitch;

	if (attr.fbtype.fb_width != ILFB_PHYSICAL_WIDTH ||
	    attr.fbtype.fb_height != ILFB_PHYSICAL_HEIGHT ||
	    attr.fbtype.fb_depth != ILFB_PHYSICAL_BPP ||
	    pitch != ILFB_PHYSICAL_PITCH) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "ROTFB requires physical %dx%d %d-bpp pitch %d; "
		    "device reports %dx%d %d-bpp pitch %u\n",
		    ILFB_PHYSICAL_WIDTH, ILFB_PHYSICAL_HEIGHT,
		    ILFB_PHYSICAL_BPP, ILFB_PHYSICAL_PITCH,
		    attr.fbtype.fb_width, attr.fbtype.fb_height,
		    attr.fbtype.fb_depth, pitch);
		close(fd);
		return (-1);
	}

	min_pitch = (uint64_t)attr.fbtype.fb_width *
	    (uint64_t)(attr.fbtype.fb_depth / 8);
	required_size = (uint64_t)pitch *
	    (uint64_t)attr.fbtype.fb_height;
	if ((attr.fbtype.fb_depth & 7) != 0 ||
	    pitch > (uint32_t)INT_MAX || (uint64_t)pitch < min_pitch ||
	    required_size > (uint64_t)attr.fbtype.fb_size ||
	    required_size != (uint64_t)ILFB_PHYSICAL_SIZE) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Invalid physical framebuffer layout: pitch %u, "
		    "size %d, required %llu\n", pitch,
		    attr.fbtype.fb_size,
		    (unsigned long long)required_size);
		close(fd);
		return (-1);
	}

	fPtr->width = attr.fbtype.fb_width;
	fPtr->height = attr.fbtype.fb_height;
	fPtr->fb_bpp = attr.fbtype.fb_depth;
	fPtr->fbmem_len = (size_t)attr.fbtype.fb_size;
	fPtr->linebytes = (int)pitch;

	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	    "Physical framebuffer: %dx%d, %d bpp, pitch %d, size %zu\n",
	    fPtr->width, fPtr->height, fPtr->fb_bpp,
	    fPtr->linebytes, fPtr->fbmem_len);
	return (fd);
}

static Bool
IlFbProbe(DriverPtr drv, int flags)
{
	int numDevSections;
	GDevPtr *devSections;
	int i, entity;
	Bool foundScreen = FALSE;

	if (flags & PROBE_DETECT)
		return (FALSE);

	numDevSections = xf86MatchDevice(ILFB_DRIVER_NAME, &devSections);
	if (numDevSections <= 0)
		return (FALSE);

	for (i = 0; i < numDevSections; i++) {
		ScrnInfoPtr pScrn = NULL;
		const char *dev;
		int fd;

		dev = xf86FindOptionValue(devSections[i]->options, "Device");
		if (dev == NULL)
			dev = ILFB_DEFAULT_DEV;

		fd = open(dev, O_RDWR);
		if (fd < 0)
			continue;
		close(fd);

		entity = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);
		pScrn = xf86ConfigFbEntity(NULL, 0, entity,
		    NULL, NULL, NULL, NULL);

		if (pScrn != NULL) {
			foundScreen = TRUE;
			pScrn->driverVersion = ILFB_VERSION;
			pScrn->driverName = ILFB_DRIVER_NAME;
			pScrn->name = ILFB_NAME;
			pScrn->Probe = IlFbProbe;
			pScrn->PreInit = IlFbPreInit;
			pScrn->ScreenInit = IlFbScreenInit;
			pScrn->SwitchMode = IlFbSwitchMode;
			pScrn->AdjustFrame = NULL;
			pScrn->EnterVT = IlFbEnterVT;
			pScrn->LeaveVT = IlFbLeaveVT;
			pScrn->ValidMode = IlFbValidMode;
		}
	}

	free(devSections);
	return (foundScreen);
}

static void
IlFbFreePrivate(ScrnInfoPtr pScrn)
{
	IlFbPtr fPtr = ILFBPTR(pScrn);

	if (fPtr == NULL)
		return;

	if (fPtr->fd >= 0) {
		close(fPtr->fd);
		fPtr->fd = -1;
	}

	free(fPtr->pEnt);
	fPtr->pEnt = NULL;

	free(fPtr->Options);
	fPtr->Options = NULL;

	free(fPtr);
	pScrn->driverPrivate = NULL;
}

static Bool
IlFbPreInit(ScrnInfoPtr pScrn, int flags)
{
	IlFbPtr fPtr;
	const char *dev;
	int defaultDepth;
	Gamma zeros = {0.0, 0.0, 0.0};
	rgb zeros_rgb = {0, 0, 0};
	DisplayModePtr mode;

	if (flags & PROBE_DETECT)
		return (FALSE);

	if (pScrn->numEntities != 1)
		return (FALSE);

	/* Allocate private data */
	if (pScrn->driverPrivate == NULL)
		pScrn->driverPrivate = xnfcalloc(sizeof (IlFbRec), 1);
	fPtr = ILFBPTR(pScrn);
	fPtr->fd = -1;

	fPtr->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

	if (pScrn->confScreen != NULL)
		pScrn->monitor = pScrn->confScreen->monitor;

	/*
	 * Skip xf86CollectOptions() — it dereferences pScrn->display
	 * unconditionally, which can be NULL for xf86ClaimFbSlot
	 * entities.  The reference drivers (wsfb, scfb) call it but
	 * then pass pEnt->device->options to xf86ProcessOptions()
	 * anyway, so its output is unused.  We go straight to the
	 * device options.
	 */
	fPtr->Options = malloc(sizeof (IlFbOptions));
	if (fPtr->Options == NULL)
		goto fail;
	memcpy(fPtr->Options, IlFbOptions, sizeof (IlFbOptions));

	if (fPtr->pEnt == NULL)
		goto fail;

	if (fPtr->pEnt->device != NULL) {
		xf86ProcessOptions(pScrn->scrnIndex,
		    fPtr->pEnt->device->options, fPtr->Options);
	}

	dev = ILFB_DEFAULT_DEV;
	if (fPtr->pEnt->device != NULL) {
		const char *optdev = xf86FindOptionValue(
		    fPtr->pEnt->device->options, "Device");
		if (optdev != NULL)
			dev = optdev;
	}

	/* Open device and query parameters */
	fPtr->fd = IlFbOpenDev(pScrn, dev, fPtr);
	if (fPtr->fd < 0)
		goto fail;

	/* ROTFB requires ShadowFB: direct framebuffer rendering is invalid. */
	fPtr->shadowFB = xf86ReturnOptValBool(fPtr->Options,
	    OPTION_SHADOW_FB, TRUE);
	if (!fPtr->shadowFB) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "ROTFB requires ShadowFB\n");
		goto fail;
	}

	/* Set depth and bpp */
	defaultDepth = fPtr->fb_bpp <= 24 ? fPtr->fb_bpp : 24;
	if (!xf86SetDepthBpp(pScrn, defaultDepth, defaultDepth,
	    fPtr->fb_bpp,
	    fPtr->fb_bpp >= 24 ? Support24bppFb | Support32bppFb : 0))
		goto fail;
	if (pScrn->depth != 24 || pScrn->bitsPerPixel != 32) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "ROTFB requires Xorg depth 24 with 32 bits per pixel\n");
		goto fail;
	}

	xf86PrintDepthBpp(pScrn);

	/* Color weight for depth > 8 */
	if (pScrn->depth > 8) {
		if (!xf86SetWeight(pScrn, zeros_rgb, zeros_rgb))
			goto fail;
	}

	/* Default visual */
	if (!xf86SetDefaultVisual(pScrn, -1))
		goto fail;

	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Default visual (%s) not supported at "
		    "depth %d\n",
		    xf86GetVisualName(pScrn->defaultVisual),
		    pScrn->depth);
		goto fail;
	}

	/* Gamma */
	if (!xf86SetGamma(pScrn, zeros))
		goto fail;

	pScrn->progClock = TRUE;
	pScrn->rgbBits = 8;
	pScrn->chipset = ILFB_NAME;
	pScrn->videoRam = fPtr->fbmem_len / 1024;

	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	    "Using ShadowFB: %s\n",
	    fPtr->shadowFB ? "Yes" : "No");

	/* Create a single fixed display mode */
	mode = xnfcalloc(sizeof (DisplayModeRec), 1);
	mode->prev = mode;
	mode->next = mode;
	mode->name = "current";
	mode->status = MODE_OK;
	mode->type = M_T_BUILTIN;
	mode->Clock = 0;
	mode->HDisplay = ILFB_LOGICAL_WIDTH;
	mode->HSyncStart = ILFB_LOGICAL_WIDTH;
	mode->HSyncEnd = ILFB_LOGICAL_WIDTH;
	mode->HTotal = ILFB_LOGICAL_WIDTH;
	mode->VDisplay = ILFB_LOGICAL_HEIGHT;
	mode->VSyncStart = ILFB_LOGICAL_HEIGHT;
	mode->VSyncEnd = ILFB_LOGICAL_HEIGHT;
	mode->VTotal = ILFB_LOGICAL_HEIGHT;

	pScrn->currentMode = pScrn->modes = mode;
	pScrn->virtualX = ILFB_LOGICAL_WIDTH;
	pScrn->virtualY = ILFB_LOGICAL_HEIGHT;
	/* displayWidth is in pixels; actual devKind is validated later. */
	pScrn->displayWidth = ILFB_LOGICAL_WIDTH;

	/* DPI */
	xf86SetDpi(pScrn, 0, 0);

	/* Load required modules */
	if (xf86LoadSubModule(pScrn, "fb") == NULL)
		goto fail;

	if (fPtr->shadowFB) {
		if (xf86LoadSubModule(pScrn, "shadow") == NULL)
			goto fail;
	}

	return (TRUE);

fail:
	IlFbFreePrivate(pScrn);
	return (FALSE);
}

/*
 * ShadowWindowProc for Xorg's shadowUpdateRotate32_270 helper.
 * The helper supplies physical row and byte offset; this callback does
 * not rotate coordinates a second time.
 */
static void *
IlFbWindowLinear(ScreenPtr pScreen, CARD32 row, CARD32 offset,
    int mode, CARD32 *size, void *closure)
{
	IlFbPtr fPtr = (IlFbPtr)closure;
	size_t row_start, byte_offset, available, row_remaining;
	const size_t visible_row_bytes = ILFB_PHYSICAL_WIDTH *
	    (ILFB_PHYSICAL_BPP / 8);

	(void)pScreen;
	if (size == NULL)
		return (NULL);
	*size = 0;

	if (fPtr == NULL || fPtr->linebytes <= 0 ||
	    mode != SHADOW_WINDOW_WRITE || !fPtr->fb_mapped ||
	    !fPtr->graphics_mode || !fPtr->vt_active ||
	    fPtr->fbmem == NULL || row >= (CARD32)fPtr->height ||
	    offset >= visible_row_bytes || (offset & 3) != 0 ||
	    offset > (CARD32)fPtr->linebytes)
		return (NULL);

	if ((size_t)row > fPtr->fbmem_len / (size_t)fPtr->linebytes)
		return (NULL);
	row_start = (size_t)row * (size_t)fPtr->linebytes;
	byte_offset = (size_t)offset;
	if (row_start > fPtr->fbmem_len ||
	    byte_offset > fPtr->fbmem_len - row_start ||
	    byte_offset > (size_t)fPtr->linebytes)
		return (NULL);

	row_remaining = (size_t)fPtr->linebytes - byte_offset;
	available = visible_row_bytes - byte_offset;
	if (available > row_remaining)
		available = row_remaining;
	if (available > fPtr->fbmem_len - row_start - byte_offset)
		available = fPtr->fbmem_len - row_start - byte_offset;
	if (available == 0 || available > UINT32_MAX)
		return (NULL);

	*size = (CARD32)available;
	return ((void *)(fPtr->fbmem + row_start + byte_offset));
}

/*
 * The old SIMD copy dispatch cannot be used here: the Xorg rotation helper
 * owns the per-pixel traversal and the logical-to-physical transform.  A
 * linear streaming copy would bypass that transform.  Keep the WC store
 * completion fence after the helper has finished writing the framebuffer.
 */
static void
IlFbRotateUpdate(ScreenPtr pScreen, shadowBufPtr pBuf)
{
	IlFbPtr fPtr = (IlFbPtr)pBuf->closure;

	if (fPtr == NULL || fPtr->rotate_update == NULL)
		return;
	(*fPtr->rotate_update)(pScreen, pBuf);
	/* Flush WC framebuffer stores before returning to the X server. */
	__asm__ __volatile__("sfence" ::: "memory");
}

static void
IlFbDamageFull(ScreenPtr pScreen, IlFbPtr fPtr)
{
	PixmapPtr pPixmap;
	BoxRec box;
	RegionRec region;

	if (pScreen == NULL || fPtr == NULL ||
	    !fPtr->screen_pixmap_ready || !fPtr->shadow_registered)
		return;
	pPixmap = pScreen->GetScreenPixmap(pScreen);
	if (pPixmap == NULL)
		return;
	box.x1 = 0;
	box.y1 = 0;
	box.x2 = ILFB_LOGICAL_WIDTH;
	box.y2 = ILFB_LOGICAL_HEIGHT;
	RegionInit(&region, &box, 1);
	DamageRegionAppend(&pPixmap->drawable, &region);
	DamageRegionProcessPending(&pPixmap->drawable);
	RegionUninit(&region);
}

static Bool
IlFbCreateScreenResources(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	IlFbPtr fPtr = ILFBPTR(pScrn);
	PixmapPtr pPixmap;
	uint64_t min_stride, total_size;
	Bool ret;

	if (fPtr->CreateScreenResources == NULL) {
		IlFbAbortAfterFbInit(pScreen, pScrn, fPtr);
		return (FALSE);
	}
	pScreen->CreateScreenResources = fPtr->CreateScreenResources;
	ret = pScreen->CreateScreenResources(pScreen);
	pScreen->CreateScreenResources = IlFbCreateScreenResources;
	if (!ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "CreateScreenResources failed; retaining referenced memory\n");
		IlFbAbortAfterFbInit(pScreen, pScrn, fPtr);
		return (FALSE);
	}

	/* miCreateScreenResources has now replaced its MI record with a pixmap. */
	fPtr->screen_pixmap_ready = TRUE;
	pPixmap = pScreen->GetScreenPixmap(pScreen);
	min_stride = (uint64_t)ILFB_LOGICAL_WIDTH *
	    (uint64_t)(pScrn->bitsPerPixel / 8);
	if (pPixmap == NULL || pPixmap->devPrivate.ptr != fPtr->shadow ||
	    pPixmap->drawable.width != ILFB_LOGICAL_WIDTH ||
	    pPixmap->drawable.height != ILFB_LOGICAL_HEIGHT ||
	    pPixmap->drawable.depth != pScrn->depth ||
	    pPixmap->drawable.bitsPerPixel != 32 ||
	    pPixmap->devKind <= 0 || (uint64_t)pPixmap->devKind < min_stride ||
	    (uint64_t)pPixmap->devKind >
	    (uint64_t)(fPtr->shadow_len / ILFB_LOGICAL_HEIGHT)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Screen pixmap layout does not fit ROTFB shadow allocation\n");
		return (FALSE);
	}
	total_size = (uint64_t)pPixmap->devKind * ILFB_LOGICAL_HEIGHT;
	if (total_size > (uint64_t)fPtr->shadow_len ||
	    total_size > SIZE_MAX) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Screen pixmap stride/height overflows shadow allocation\n");
		return (FALSE);
	}
	fPtr->shadow_stride = pPixmap->devKind;

	if (!shadowAdd(pScreen, pPixmap, IlFbRotateUpdate,
	    IlFbWindowLinear, 270, fPtr)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "shadowAdd failed\n");
		return (FALSE);
	}
	fPtr->shadow_registered = TRUE;
	IlFbDamageFull(pScreen, fPtr);
	return (TRUE);
}

static Bool
IlFbShadowInit(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	IlFbPtr fPtr = ILFBPTR(pScrn);

	if (!shadowSetup(pScreen))
		return (FALSE);
	fPtr->shadow_setup = TRUE;
	fPtr->CreateScreenResources = pScreen->CreateScreenResources;
	pScreen->CreateScreenResources = IlFbCreateScreenResources;
	return (TRUE);
}

static Bool
IlFbFailedCloseScreen(ScreenPtr pScreen)
{
	/*
	 * Xorg 1.19.7 has no rollback for partially completed shadowSetup(),
	 * miDCInitialize(), or miCreateScreenResources().  In this terminal
	 * failure state, calling the partially installed CloseScreen chain can
	 * dereference freed private data or treat the MI init record as a pixmap.
	 * Deliberately retain possibly referenced allocations until server exit.
	 */
	(void)pScreen;
	return (TRUE);
}

static void
IlFbAbortAfterFbInit(ScreenPtr pScreen, ScrnInfoPtr pScrn, IlFbPtr fPtr)
{
	fPtr->terminal_failure = TRUE;
	fPtr->vt_active = FALSE;
	pScrn->vtSema = FALSE;
	if (fPtr->graphics_mode && ioctl(fPtr->fd, KDSETMODE, KD_TEXT) == 0)
		fPtr->graphics_mode = FALSE;
	pScrn->vtSema = fPtr->graphics_mode;
	pScreen->CloseScreen = IlFbFailedCloseScreen;
}

static void
IlFbReleasePreInitScreenResources(ScrnInfoPtr pScrn, IlFbPtr fPtr)
{
	if (fPtr->graphics_mode) {
		if (ioctl(fPtr->fd, KDSETMODE, KD_TEXT) == 0)
			fPtr->graphics_mode = FALSE;
		else
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			    "KDSETMODE KD_TEXT after ScreenInit failure: %s\n",
			    strerror(errno));
	}
	fPtr->vt_active = FALSE;
	pScrn->vtSema = fPtr->graphics_mode;
	if (fPtr->fb_mapped && !fPtr->graphics_mode) {
		if (munmap(fPtr->fbmem, fPtr->fbmem_len) == 0) {
			fPtr->fbmem = NULL;
			fPtr->fb_mapped = FALSE;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			    "munmap after ScreenInit failure: %s\n",
			    strerror(errno));
		}
	}
}

static Bool
IlFbScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	IlFbPtr fPtr = ILFBPTR(pScrn);
	VisualPtr visual;
	int shadow_stride;
	size_t shadow_len;

	(void)argc;
	(void)argv;
	if (fPtr->terminal_failure)
		return (FALSE);

	if (!fPtr->fb_mapped) {
		fPtr->fbmem = mmap(NULL, fPtr->fbmem_len,
		    PROT_READ | PROT_WRITE, MAP_SHARED, fPtr->fd, 0);
		if (fPtr->fbmem == MAP_FAILED) {
			fPtr->fbmem = NULL;
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			    "mmap failed: %s\n", strerror(errno));
			return (FALSE);
		}
		fPtr->fb_mapped = TRUE;
	}

	if (ioctl(fPtr->fd, KDSETMODE, KD_GRAPHICS) != 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "KDSETMODE KD_GRAPHICS failed: %s\n", strerror(errno));
		IlFbReleasePreInitScreenResources(pScrn, fPtr);
		return (FALSE);
	}
	fPtr->graphics_mode = TRUE;
	fPtr->vt_active = TRUE;
	pScrn->vtSema = TRUE;

	fPtr->rotate_update = (shadowUpdateProc)
	    LoaderSymbol("shadowUpdateRotate32_270");
	if (fPtr->rotate_update == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "shadowUpdateRotate32_270 is not exported by shadow module\n");
		IlFbReleasePreInitScreenResources(pScrn, fPtr);
		return (FALSE);
	}

	miClearVisualTypes();
	if (pScrn->bitsPerPixel > 8) {
		if (!miSetVisualTypes(pScrn->depth, TrueColorMask,
		    pScrn->rgbBits, TrueColor)) {
			IlFbReleasePreInitScreenResources(pScrn, fPtr);
			return (FALSE);
		}
	} else if (!miSetVisualTypes(pScrn->depth,
	    miGetDefaultVisualMask(pScrn->depth), pScrn->rgbBits,
	    pScrn->defaultVisual)) {
		IlFbReleasePreInitScreenResources(pScrn, fPtr);
		return (FALSE);
	}
	if (!miSetPixmapDepths()) {
		IlFbReleasePreInitScreenResources(pScrn, fPtr);
		return (FALSE);
	}

	/*
	 * Allocate using Xorg's pixmap padding rule.  The actual stride used
	 * later is pPixmap->devKind and is checked against this allocation.
	 */
	shadow_stride = PixmapBytePad(ILFB_LOGICAL_WIDTH, pScrn->depth);
	if (shadow_stride <= 0 ||
	    (size_t)shadow_stride > SIZE_MAX / ILFB_LOGICAL_HEIGHT) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Invalid logical shadow stride\n");
		IlFbReleasePreInitScreenResources(pScrn, fPtr);
		return (FALSE);
	}
	shadow_len = (size_t)shadow_stride * ILFB_LOGICAL_HEIGHT;
	fPtr->shadow = calloc(1, shadow_len);
	if (fPtr->shadow == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Failed to allocate logical shadow buffer\n");
		IlFbReleasePreInitScreenResources(pScrn, fPtr);
		return (FALSE);
	}
	fPtr->shadow_len = shadow_len;

	if (!fbScreenInit(pScreen, fPtr->shadow,
	    ILFB_LOGICAL_WIDTH, ILFB_LOGICAL_HEIGHT,
	    pScrn->xDpi, pScrn->yDpi, ILFB_LOGICAL_WIDTH,
	    pScrn->bitsPerPixel)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "fbScreenInit failed; retaining possibly referenced shadow memory\n");
		IlFbAbortAfterFbInit(pScreen, pScrn, fPtr);
		return (FALSE);
	}
	fPtr->fb_initialized = TRUE;

	if (pScrn->bitsPerPixel > 8) {
		visual = pScreen->visuals + pScreen->numVisuals;
		while (--visual >= pScreen->visuals) {
			if ((visual->class | DynamicClass) == DirectColor) {
				visual->offsetRed = pScrn->offset.red;
				visual->offsetGreen = pScrn->offset.green;
				visual->offsetBlue = pScrn->offset.blue;
				visual->redMask = pScrn->mask.red;
				visual->greenMask = pScrn->mask.green;
				visual->blueMask = pScrn->mask.blue;
			}
		}
	}

	if (pScrn->bitsPerPixel >= 8 &&
	    !fbPictureInit(pScreen, NULL, 0))
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "RENDER extension init failed\n");

	if (!IlFbShadowInit(pScreen)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "shadowSetup failed; retaining resources because Xorg 1.19.7 "
		    "does not provide complete rollback\n");
		IlFbAbortAfterFbInit(pScreen, pScrn, fPtr);
		return (FALSE);
	}

	xf86SetBlackWhitePixels(pScreen);
	xf86SetBackingStore(pScreen);
	if (!miDCInitialize(pScreen, xf86GetPointerScreenFuncs())) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "miDCInitialize failed; bypassing unsafe partial CloseScreen chain\n");
		IlFbAbortAfterFbInit(pScreen, pScrn, fPtr);
		return (FALSE);
	}

	if (!miCreateDefColormap(pScreen)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "miCreateDefColormap failed; retaining screen resources\n");
		IlFbAbortAfterFbInit(pScreen, pScrn, fPtr);
		return (FALSE);
	}
	pScreen->SaveScreen = IlFbSaveScreen;

	fPtr->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = IlFbCloseScreen;
	return (TRUE);
}

static Bool
IlFbCloseScreen(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	IlFbPtr fPtr = ILFBPTR(pScrn);
	Bool ret;

	if (fPtr->terminal_failure || !fPtr->screen_pixmap_ready ||
	    fPtr->CloseScreen == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "CloseScreen bypassed: incomplete Xorg resource lifecycle; "
		    "retaining allocations\n");
		IlFbAbortAfterFbInit(pScreen, pScrn, fPtr);
		return (IlFbFailedCloseScreen(pScreen));
	}

	/* Stop new shadow writes, but keep both buffers alive through teardown. */
	fPtr->vt_active = FALSE;
	if (fPtr->graphics_mode) {
		if (ioctl(fPtr->fd, KDSETMODE, KD_TEXT) == 0)
			fPtr->graphics_mode = FALSE;
		else
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			    "KDSETMODE KD_TEXT during CloseScreen: %s\n",
			    strerror(errno));
	}
	if (!fPtr->graphics_mode)
		pScrn->vtSema = FALSE;

	pScreen->CloseScreen = fPtr->CloseScreen;
	ret = (*pScreen->CloseScreen)(pScreen);

	/* shadowCloseScreen/fbCloseScreen have now released pixmap references. */
	fPtr->shadow_registered = FALSE;
	fPtr->shadow_setup = FALSE;
	fPtr->screen_pixmap_ready = FALSE;
	fPtr->fb_initialized = FALSE;
	if (fPtr->shadow != NULL) {
		free(fPtr->shadow);
		fPtr->shadow = NULL;
		fPtr->shadow_len = 0;
		fPtr->shadow_stride = 0;
	}
	if (fPtr->fb_mapped && !fPtr->graphics_mode) {
		if (munmap(fPtr->fbmem, fPtr->fbmem_len) == 0) {
			fPtr->fbmem = NULL;
			fPtr->fb_mapped = FALSE;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			    "munmap during CloseScreen: %s\n", strerror(errno));
		}
	}
	return (ret);
}

static Bool
IlFbEnterVT(ScrnInfoPtr pScrn)
{
	IlFbPtr fPtr = ILFBPTR(pScrn);
	Bool newly_mapped = FALSE;

	if (fPtr->terminal_failure)
		return (FALSE);
	if (!fPtr->fb_mapped) {
		fPtr->fbmem = mmap(NULL, fPtr->fbmem_len,
		    PROT_READ | PROT_WRITE, MAP_SHARED, fPtr->fd, 0);
		if (fPtr->fbmem == MAP_FAILED) {
			fPtr->fbmem = NULL;
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			    "EnterVT mmap failed: %s\n", strerror(errno));
			return (FALSE);
		}
		fPtr->fb_mapped = TRUE;
		newly_mapped = TRUE;
	}
	if (ioctl(fPtr->fd, KDSETMODE, KD_GRAPHICS) != 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "EnterVT KDSETMODE KD_GRAPHICS failed: %s\n",
		    strerror(errno));
		if (newly_mapped && munmap(fPtr->fbmem, fPtr->fbmem_len) == 0) {
			fPtr->fbmem = NULL;
			fPtr->fb_mapped = FALSE;
		}
		return (FALSE);
	}
	fPtr->graphics_mode = TRUE;
	fPtr->vt_active = TRUE;
	pScrn->vtSema = TRUE;
	IlFbDamageFull(pScrn->pScreen, fPtr);
	return (TRUE);
}

static void
IlFbLeaveVT(ScrnInfoPtr pScrn)
{
	IlFbPtr fPtr = ILFBPTR(pScrn);

	if (fPtr->graphics_mode && ioctl(fPtr->fd, KDSETMODE, KD_TEXT) != 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "LeaveVT KDSETMODE KD_TEXT failed: %s\n", strerror(errno));
		/* Keep VT ownership and mapping state truthful on ioctl failure. */
		return;
	}
	fPtr->graphics_mode = FALSE;
	fPtr->vt_active = FALSE;
	pScrn->vtSema = FALSE;
	if (fPtr->fb_mapped) {
		if (munmap(fPtr->fbmem, fPtr->fbmem_len) == 0) {
			fPtr->fbmem = NULL;
			fPtr->fb_mapped = FALSE;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			    "LeaveVT munmap failed; retaining mapping: %s\n",
			    strerror(errno));
		}
	}
}

static Bool
IlFbSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	return (TRUE);
}

static ModeStatus
IlFbValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode, Bool verbose, int flags)
{
	return (MODE_OK);
}

static Bool
IlFbSaveScreen(ScreenPtr pScreen, int mode)
{
	return (TRUE);
}
