/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2026
 *
 * Diagnostic-only Intel LPSS I2C PCI driver for illumos.
 *
 * This first-stage module is intentionally read-only:
 *   - maps PCI BAR0 (PCI register set 1);
 *   - reads DesignWare identification/status/FIFO-level registers;
 *   - publishes a snapshot through kstat;
 *   - optionally allocates (but does not add a handler to or enable)
 *     one DDI interrupt handle, solely to report whether the nexus can
 *     provide an interrupt resource.
 *
 * It does NOT:
 *   - write any MMIO register;
 *   - touch IC_ENABLE;
 *   - issue I2C transactions;
 *   - register an I2C controller provider;
 *   - create a character minor or expose ioctl operations.
 *
 * PCI matching is intentionally not hard-coded. Add a driver alias only
 * after the actual PCI device-id has been collected from the target laptop.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/modctl.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/ddi_intr.h>
#include <sys/kstat.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/stat.h>

#define LPSS_DRIVER_NAME             "intel_lpss_i2c"

/*
 * Per pci(5)/pcie(5), PCI register set 0 is the configuration header.
 * BAR0 is register set 1.
 */
#define LPSS_BAR0_RNUMBER            1

#define DW_IC_STATUS                 0x70
#define DW_IC_TXFLR                  0x78
#define DW_IC_RXFLR                  0x7c
#define DW_IC_COMP_TYPE              0xf4
#define DW_IC_COMP_VERSION           0xf8
#define DW_IC_COMP_PARAM_1           0xfc

#define LPSS_BAR0_MIN_SIZE           0x100
#define DW_IC_COMP_TYPE_EXPECTED     0x44570140U

#define LPSS_KSTAT_NDATA             15

typedef struct intel_lpss_i2c {
	dev_info_t		*sc_dip;
	int			sc_instance;

	int			sc_vendor_id;
	int			sc_device_id;
	uint_t			sc_reg_cells;

	off_t			sc_bar0_size;
	caddr_t			sc_regs;
	ddi_acc_handle_t	sc_reg_handle;
	boolean_t		sc_regs_mapped;

	/*
	 * Optional allocation-only interrupt probe. No handler is added and
	 * the interrupt is never enabled in this diagnostic driver.
	 */
	ddi_intr_handle_t	sc_intr_handle;
	boolean_t		sc_intr_allocated;
	int			sc_intr_type;
	uint_t			sc_intr_pri;
	int			sc_intr_caps;
	int			sc_intr_alloc_status;

	uint32_t		sc_comp_type;
	uint32_t		sc_comp_version;
	uint32_t		sc_comp_param_1;
	uint32_t		sc_status;
	uint32_t		sc_txflr;
	uint32_t		sc_rxflr;

	kstat_t			*sc_ksp;
} intel_lpss_i2c_t;

static void *intel_lpss_i2c_statep;

/*
 * These are the requested access attributes for the DesignWare MMIO block.
 * No writes are performed by this module.
 */
static ddi_device_acc_attr_t intel_lpss_i2c_acc_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_STRUCTURE_LE_ACC,
	DDI_STRICTORDER_ACC
};

static int intel_lpss_i2c_attach(dev_info_t *, ddi_attach_cmd_t);
static int intel_lpss_i2c_detach(dev_info_t *, ddi_detach_cmd_t);

static struct dev_ops intel_lpss_i2c_dev_ops = {
	.devo_rev	= DEVO_REV,
	.devo_refcnt	= 0,
	.devo_getinfo	= nodev,
	.devo_identify	= nulldev,
	.devo_probe	= nulldev,
	.devo_attach	= intel_lpss_i2c_attach,
	.devo_detach	= intel_lpss_i2c_detach,
	.devo_reset	= nodev,
	.devo_cb_ops	= NULL,
	.devo_bus_ops	= NULL,
	.devo_power	= NULL,
	.devo_quiesce	= ddi_quiesce_not_needed
};

static struct modldrv intel_lpss_i2c_modldrv = {
	.drv_modops	= &mod_driverops,
	.drv_linkinfo	= "Diagnostic Intel LPSS I2C register probe",
	.drv_dev_ops	= &intel_lpss_i2c_dev_ops
};

static struct modlinkage intel_lpss_i2c_modlinkage = {
	.ml_rev		= MODREV_1,
	.ml_linkage	= { &intel_lpss_i2c_modldrv, NULL }
};

static uint32_t
intel_lpss_i2c_get32(intel_lpss_i2c_t *sc, uint32_t offset)
{
	return (ddi_get32(sc->sc_reg_handle,
	    (uint32_t *)(void *)(sc->sc_regs + offset)));
}

/*
 * Record the PCI "reg" property length for diagnostics. This is an integer
 * array property. The string "pci8086,XXXX" is a compatible/binding name,
 * NOT an integer-array property, and must not be passed to
 * ddi_prop_lookup_int_array().
 */
static void
intel_lpss_i2c_read_pci_properties(intel_lpss_i2c_t *sc)
{
	int *regp = NULL;
	uint_t nreg = 0;
	char **compat = NULL;
	uint_t ncompat = 0;
	uint_t i;

	sc->sc_vendor_id = ddi_prop_get_int(DDI_DEV_T_ANY, sc->sc_dip,
	    DDI_PROP_DONTPASS, "vendor-id", -1);
	sc->sc_device_id = ddi_prop_get_int(DDI_DEV_T_ANY, sc->sc_dip,
	    DDI_PROP_DONTPASS, "device-id", -1);

	if (ddi_prop_lookup_int_array(DDI_DEV_T_ANY, sc->sc_dip,
	    DDI_PROP_DONTPASS, "reg", &regp, &nreg) == DDI_PROP_SUCCESS) {
		sc->sc_reg_cells = nreg;
		ddi_prop_free(regp);
	}

	/*
	 * "compatible" is a string array. Log it to make the eventual
	 * driver_aliases entry auditable after the hardware ID is known.
	 */
	if (ddi_prop_lookup_string_array(DDI_DEV_T_ANY, sc->sc_dip,
	    DDI_PROP_DONTPASS, "compatible", &compat, &ncompat) ==
	    DDI_PROP_SUCCESS) {
		for (i = 0; i < ncompat; i++) {
			cmn_err(CE_NOTE, "%s%d: compatible[%u]=%s",
			    LPSS_DRIVER_NAME, sc->sc_instance, i, compat[i]);
		}
		ddi_prop_free(compat);
	}

	cmn_err(CE_NOTE, "%s%d: PCI vendor-id=0x%x device-id=0x%x reg-cells=%u",
	    LPSS_DRIVER_NAME, sc->sc_instance,
	    (uint32_t)sc->sc_vendor_id, (uint32_t)sc->sc_device_id,
	    sc->sc_reg_cells);

	if (sc->sc_device_id == -1) {
		cmn_err(CE_WARN, "%s%d: device-id property unavailable; "
		    "PCI binding alias still requires hardware inventory",
		    LPSS_DRIVER_NAME, sc->sc_instance);
	}
}

/*
 * Probe the interrupt resource without installing or enabling an ISR.
 * This is optional: lack of an allocatable interrupt must not prevent this
 * read-only register diagnostic from attaching.
 */
static int
intel_lpss_i2c_try_intr_type(intel_lpss_i2c_t *sc, int type)
{
	int nintrs = 0;
	int navail = 0;
	int actual = 0;
	int ret;

	ret = ddi_intr_get_nintrs(sc->sc_dip, type, &nintrs);
	if (ret != DDI_SUCCESS || nintrs < 1)
		return (DDI_FAILURE);

	ret = ddi_intr_get_navail(sc->sc_dip, type, &navail);
	if (ret != DDI_SUCCESS || navail < 1)
		return (DDI_FAILURE);

	sc->sc_intr_handle = NULL;
	ret = ddi_intr_alloc(sc->sc_dip, &sc->sc_intr_handle, type,
	    0, 1, &actual, DDI_INTR_ALLOC_NORMAL);

	if (ret != DDI_SUCCESS || actual != 1) {
		if (actual > 0 && sc->sc_intr_handle != NULL)
			(void) ddi_intr_free(sc->sc_intr_handle);
		sc->sc_intr_handle = NULL;
		return (DDI_FAILURE);
	}

	sc->sc_intr_allocated = B_TRUE;
	sc->sc_intr_type = type;

	ret = ddi_intr_get_pri(sc->sc_intr_handle, &sc->sc_intr_pri);
	if (ret != DDI_SUCCESS)
		goto fail;

	ret = ddi_intr_get_cap(sc->sc_intr_handle, &sc->sc_intr_caps);
	if (ret != DDI_SUCCESS)
		goto fail;

	return (DDI_SUCCESS);

fail:
	(void) ddi_intr_free(sc->sc_intr_handle);
	sc->sc_intr_handle = NULL;
	sc->sc_intr_allocated = B_FALSE;
	sc->sc_intr_type = 0;
	sc->sc_intr_pri = 0;
	sc->sc_intr_caps = 0;
	return (DDI_FAILURE);
}

static void
intel_lpss_i2c_probe_interrupt(intel_lpss_i2c_t *sc)
{
	int types = 0;

	sc->sc_intr_alloc_status = DDI_FAILURE;

	if (ddi_intr_get_supported_types(sc->sc_dip, &types) != DDI_SUCCESS)
		return;

	if ((types & DDI_INTR_TYPE_MSI) != 0 &&
	    intel_lpss_i2c_try_intr_type(sc, DDI_INTR_TYPE_MSI) ==
	    DDI_SUCCESS) {
		sc->sc_intr_alloc_status = DDI_SUCCESS;
		return;
	}

	if ((types & DDI_INTR_TYPE_FIXED) != 0 &&
	    intel_lpss_i2c_try_intr_type(sc, DDI_INTR_TYPE_FIXED) ==
	    DDI_SUCCESS) {
		sc->sc_intr_alloc_status = DDI_SUCCESS;
		return;
	}
}

/*
 * Snapshot only the explicitly approved read-only registers.
 * IC_ENABLE and every other control register are deliberately untouched.
 */
static void
intel_lpss_i2c_read_snapshot(intel_lpss_i2c_t *sc)
{
	sc->sc_comp_type = intel_lpss_i2c_get32(sc, DW_IC_COMP_TYPE);
	sc->sc_comp_version = intel_lpss_i2c_get32(sc, DW_IC_COMP_VERSION);
	sc->sc_comp_param_1 = intel_lpss_i2c_get32(sc, DW_IC_COMP_PARAM_1);
	sc->sc_status = intel_lpss_i2c_get32(sc, DW_IC_STATUS);
	sc->sc_txflr = intel_lpss_i2c_get32(sc, DW_IC_TXFLR);
	sc->sc_rxflr = intel_lpss_i2c_get32(sc, DW_IC_RXFLR);

	if (sc->sc_comp_type != DW_IC_COMP_TYPE_EXPECTED) {
		cmn_err(CE_WARN, "%s%d: IC_COMP_TYPE=0x%08x, expected "
		    "0x%08x; preserving snapshot without touching hardware",
		    LPSS_DRIVER_NAME, sc->sc_instance, sc->sc_comp_type,
		    DW_IC_COMP_TYPE_EXPECTED);
	}
}

static int
intel_lpss_i2c_kstat_init(intel_lpss_i2c_t *sc)
{
	kstat_t *ksp;
	kstat_named_t *kn;

	ksp = kstat_create(LPSS_DRIVER_NAME, sc->sc_instance,
	    "registers", "misc", KSTAT_TYPE_NAMED, LPSS_KSTAT_NDATA, 0);
	if (ksp == NULL)
		return (ENOMEM);

	sc->sc_ksp = ksp;
	kn = (kstat_named_t *)ksp->ks_data;

	kstat_named_init(&kn[0], "vendor_id", KSTAT_DATA_UINT32);
	kn[0].value.ui32 = (uint32_t)sc->sc_vendor_id;

	kstat_named_init(&kn[1], "device_id", KSTAT_DATA_UINT32);
	kn[1].value.ui32 = (uint32_t)sc->sc_device_id;

	kstat_named_init(&kn[2], "bar0_size", KSTAT_DATA_UINT64);
	kn[2].value.ui64 = (uint64_t)sc->sc_bar0_size;

	kstat_named_init(&kn[3], "reg_cells", KSTAT_DATA_UINT32);
	kn[3].value.ui32 = sc->sc_reg_cells;

	kstat_named_init(&kn[4], "comp_type", KSTAT_DATA_UINT32);
	kn[4].value.ui32 = sc->sc_comp_type;

	kstat_named_init(&kn[5], "comp_version", KSTAT_DATA_UINT32);
	kn[5].value.ui32 = sc->sc_comp_version;

	kstat_named_init(&kn[6], "comp_param_1", KSTAT_DATA_UINT32);
	kn[6].value.ui32 = sc->sc_comp_param_1;

	kstat_named_init(&kn[7], "status", KSTAT_DATA_UINT32);
	kn[7].value.ui32 = sc->sc_status;

	kstat_named_init(&kn[8], "txflr", KSTAT_DATA_UINT32);
	kn[8].value.ui32 = sc->sc_txflr;

	kstat_named_init(&kn[9], "rxflr", KSTAT_DATA_UINT32);
	kn[9].value.ui32 = sc->sc_rxflr;

	kstat_named_init(&kn[10], "intr_allocated", KSTAT_DATA_UINT32);
	kn[10].value.ui32 = sc->sc_intr_allocated ? 1 : 0;

	kstat_named_init(&kn[11], "intr_type", KSTAT_DATA_UINT32);
	kn[11].value.ui32 = (uint32_t)sc->sc_intr_type;

	kstat_named_init(&kn[12], "intr_pri", KSTAT_DATA_UINT32);
	kn[12].value.ui32 = (uint32_t)sc->sc_intr_pri;

	kstat_named_init(&kn[13], "intr_caps", KSTAT_DATA_UINT32);
	kn[13].value.ui32 = (uint32_t)sc->sc_intr_caps;

	kstat_named_init(&kn[14], "intr_alloc_status", KSTAT_DATA_UINT32);
	kn[14].value.ui32 = (uint32_t)sc->sc_intr_alloc_status;

	kstat_install(ksp);
	return (0);
}

/*
 * Release resources in reverse acquisition order.
 */
static void
intel_lpss_i2c_cleanup(intel_lpss_i2c_t *sc)
{
	if (sc->sc_ksp != NULL) {
		kstat_delete(sc->sc_ksp);
		sc->sc_ksp = NULL;
	}

	if (sc->sc_intr_allocated) {
		(void) ddi_intr_free(sc->sc_intr_handle);
		sc->sc_intr_handle = NULL;
		sc->sc_intr_allocated = B_FALSE;
	}

	if (sc->sc_regs_mapped) {
		ddi_regs_map_free(&sc->sc_reg_handle);
		sc->sc_regs = NULL;
		sc->sc_regs_mapped = B_FALSE;
	}
}

static int
intel_lpss_i2c_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int instance;
	int ret;
	intel_lpss_i2c_t *sc;

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		/* This diagnostic driver never changes controller state. */
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	instance = ddi_get_instance(dip);
	if (ddi_soft_state_zalloc(intel_lpss_i2c_statep, instance) !=
	    DDI_SUCCESS) {
		cmn_err(CE_WARN, "%s: soft-state allocation failed for %d",
		    LPSS_DRIVER_NAME, instance);
		return (DDI_FAILURE);
	}

	sc = ddi_get_soft_state(intel_lpss_i2c_statep, instance);
	if (sc == NULL) {
		ddi_soft_state_free(intel_lpss_i2c_statep, instance);
		return (DDI_FAILURE);
	}

	sc->sc_dip = dip;
	sc->sc_instance = instance;
	sc->sc_intr_alloc_status = DDI_FAILURE;

	intel_lpss_i2c_read_pci_properties(sc);

	/*
	 * PCI register set 1 is BAR0. Obtain its size from DDI rather than
	 * embedding a physical address or a BAR size in the driver.
	 */
	ret = ddi_dev_regsize(dip, LPSS_BAR0_RNUMBER, &sc->sc_bar0_size);
	if (ret != DDI_SUCCESS || sc->sc_bar0_size < LPSS_BAR0_MIN_SIZE) {
		cmn_err(CE_WARN, "%s%d: BAR0 register set unavailable or too "
		    "small (size=%lld)", LPSS_DRIVER_NAME, instance,
		    (longlong_t)sc->sc_bar0_size);
		goto fail;
	}

	ret = ddi_regs_map_setup(dip, LPSS_BAR0_RNUMBER, &sc->sc_regs,
	    0, sc->sc_bar0_size, &intel_lpss_i2c_acc_attr,
	    &sc->sc_reg_handle);
	if (ret != DDI_SUCCESS) {
		cmn_err(CE_WARN, "%s%d: ddi_regs_map_setup(BAR0) failed: %d",
		    LPSS_DRIVER_NAME, instance, ret);
		goto fail;
	}
	sc->sc_regs_mapped = B_TRUE;

	/*
	 * Best-effort DDI interrupt resource allocation after MMIO mapping.
	 * Do not add an ISR and do not enable the interrupt in this
	 * diagnostic-only module.
	 */
	intel_lpss_i2c_probe_interrupt(sc);

	/*
	 * Read-only snapshot. No writes, no controller enable, no I2C traffic.
	 */
	intel_lpss_i2c_read_snapshot(sc);

	if (intel_lpss_i2c_kstat_init(sc) != 0) {
		cmn_err(CE_WARN, "%s%d: kstat creation failed",
		    LPSS_DRIVER_NAME, instance);
		goto fail;
	}

	ddi_report_dev(dip);
	return (DDI_SUCCESS);

fail:
	intel_lpss_i2c_cleanup(sc);
	ddi_soft_state_free(intel_lpss_i2c_statep, instance);
	return (DDI_FAILURE);
}

static int
intel_lpss_i2c_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int instance;
	intel_lpss_i2c_t *sc;

	switch (cmd) {
	case DDI_SUSPEND:
		return (DDI_SUCCESS);
	case DDI_DETACH:
		break;
	default:
		return (DDI_FAILURE);
	}

	instance = ddi_get_instance(dip);
	sc = ddi_get_soft_state(intel_lpss_i2c_statep, instance);
	if (sc == NULL)
		return (DDI_FAILURE);

	intel_lpss_i2c_cleanup(sc);
	ddi_soft_state_free(intel_lpss_i2c_statep, instance);
	return (DDI_SUCCESS);
}

int
_init(void)
{
	int ret;

	ret = ddi_soft_state_init(&intel_lpss_i2c_statep,
	    sizeof (intel_lpss_i2c_t), 1);
	if (ret != 0)
		return (ret);

	ret = mod_install(&intel_lpss_i2c_modlinkage);
	if (ret != 0)
		ddi_soft_state_fini(&intel_lpss_i2c_statep);

	return (ret);
}

int
_fini(void)
{
	int ret;

	ret = mod_remove(&intel_lpss_i2c_modlinkage);
	if (ret != 0)
		return (ret);

	ddi_soft_state_fini(&intel_lpss_i2c_statep);
	return (0);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&intel_lpss_i2c_modlinkage, modinfop));
}
