/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#ifndef VALKEYTCL_TCLVALKEYCMDSUB_H
#define VALKEYTCL_TCLVALKEYCMDSUB_H

#include "common.h"
#include "tclvalkeyCtx.h"

typedef valkeyReply* (vktcl_SubCmdProc)(vktcl_CtxType *ctx, Tcl_Interp *interp,
    int command_words, int objc, Tcl_Obj *const objv[]);

#ifdef __cplusplus
extern "C" {
#endif

vktcl_SubCmdProc vktcl_CtxHandleCmdSubRaw;
vktcl_SubCmdProc vktcl_CtxHandleCmdSubUniversal;
vktcl_SubCmdProc vktcl_CtxHandleCmdSubConfigure;

#ifdef __cplusplus
}
#endif

#endif // VALKEYTCL_TCLVALKEYCMDSUB_H
