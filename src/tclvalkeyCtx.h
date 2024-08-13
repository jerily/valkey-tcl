/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#ifndef VALKEYTCL_TCLVALKEYCTX_H
#define VALKEYTCL_TCLVALKEYCTX_H

#include "common.h"

typedef struct {
    valkeyContext *vk_ctx;
    Tcl_Interp *interp;
    char cmd[64]; /* "::valkey::handle%p" = 16 + "0x" + pointer + null = 64 bytes should be enough */
    Tcl_Command cmdToken;
    int refcount;
    Tcl_Mutex mx;

    /* options */
    int isReplyTyped;
    int isBlocking;

} vktcl_CtxType;

#define vktcl_CtxLock(x) Tcl_MutexLock(&(x)->mx);
#define vktcl_CtxUnlock(x) Tcl_MutexUnlock(&(x)->mx);
#define vktcl_CtxIsAlive(x) ((x)->vk_ctx != NULL)

#ifdef __cplusplus
extern "C" {
#endif

void vktcl_CtxPackageInitialize(void);

vktcl_CtxType *vktcl_CtxNew(Tcl_Interp *interp, valkeyContext *vk_ctx);

void vktcl_CtxClose(vktcl_CtxType *ctx);

void vktcl_CtxIncrRefCount(vktcl_CtxType *ctx);
void vktcl_CtxDecrRefCount(vktcl_CtxType *ctx);

#ifdef __cplusplus
}
#endif

#endif // VALKEYTCL_TCLVALKEYCTX_H
