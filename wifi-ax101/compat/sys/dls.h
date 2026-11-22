/*
 * Minimal illumos DLS public constant shim for the iwx GLDv3 adapter.
 *
 * The OpenIndiana userland proto does not install <sys/dls.h>; the only
 * symbol consumed by iwm_gld.c is DLS_SAP_LLC.  This value is copied from
 * illumos-gate usr/src/uts/common/sys/dls.h (CDDL), where it is defined as 0.
 * Keep this shim deliberately narrow: do not shadow the rest of sys/dls.h.
 */
#ifndef _SYS_DLS_H
#define _SYS_DLS_H
#define DLS_SAP_LLC 0
#endif
