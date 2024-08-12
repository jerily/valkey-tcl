/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */

#include "tclvalkeyCtx.h"

static int vktcl_ctx_ht_initialized = 0;

static Tcl_HashTable vktcl_ctx2internal_ht;
static Tcl_Mutex vktcl_ctx2internal_mx = NULL;

vktcl_CtxType *vktcl_CtxNew(Tcl_Interp *interp, valkeyContext *vk_ctx) {

    vktcl_CtxType *rc = ckalloc(sizeof(vktcl_CtxType));
    if (rc == NULL) {
        DBG2(printf("return: ERROR (NULL)"));
        return NULL;
    }

    rc->mx = NULL;
    rc->refcount = 0;
    rc->vk_ctx = vk_ctx;
    rc->interp = interp;
    rc->cmdToken = NULL;
    snprintf(rc->cmd, sizeof(rc->cmd), "::valkey::handle%p", (void *)rc);

    Tcl_MutexLock(&vktcl_ctx2internal_mx);

    int newEntry;
    Tcl_HashEntry *entryPtr = Tcl_CreateHashEntry(&vktcl_ctx2internal_ht, rc->cmd, &newEntry);
    Tcl_SetHashValue(entryPtr, (ClientData)rc);

    Tcl_MutexUnlock(&vktcl_ctx2internal_mx);

    DBG2(printf("return: %p", (void *)rc));

    return rc;

}

static void vktcl_CtxRemove(vktcl_CtxType *ctx) {

    Tcl_MutexLock(&vktcl_ctx2internal_mx);

    if (vktcl_ctx_ht_initialized) {
        Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&vktcl_ctx2internal_ht, ctx->cmd);
        if (entryPtr != NULL) {
            DBG2(printf("removed: %p", (void *)ctx));
            Tcl_DeleteHashEntry(entryPtr);
        } else {
            DBG2(printf("ERROR: not found %p", (void *)ctx));
        }
    } else {
        DBG2(printf("hashtable is not initialized"));
    }

    Tcl_MutexUnlock(&vktcl_ctx2internal_mx);

    return;

}

void vktcl_CtxClose(vktcl_CtxType *ctx) {
    vktcl_CtxLock(ctx);
    if (ctx->vk_ctx != NULL) {
        DBG2(printf("closed %p", (void *)ctx));
        valkeyFree(ctx->vk_ctx);
        ctx->vk_ctx = NULL;
    }
    vktcl_CtxUnlock(ctx);
    return;
}

void vktcl_CtxIncrRefCount(vktcl_CtxType *ctx) {
    vktcl_CtxLock(ctx);
    ctx->refcount++;
    DBG2(printf("refcount on %p: %d", (void *)ctx, ctx->refcount));
    vktcl_CtxUnlock(ctx);
    return;
}

void vktcl_CtxDecrRefCount(vktcl_CtxType *ctx) {

    vktcl_CtxLock(ctx);

    assert(ctx->refcount > 0 && "vktcl_CtxDecrRefCount() must not be called on context with refcount = 0");

    ctx->refcount--;
    vktcl_CtxUnlock(ctx);

    // If context is in use somewhere, just show debug message and return
    if (ctx->refcount) {
        DBG2(printf("refcount on %p: %d", (void *)ctx, ctx->refcount));
        return;
    }

    DBG2(printf("release %p ...", (void *)ctx));
    vktcl_CtxClose(ctx);
    vktcl_CtxRemove(ctx);
    Tcl_MutexFinalize(&ctx->mx);

    ckfree(ctx);
    DBG2(printf("freed %p", (void *)ctx));

    return;

}

static void vktcl_CtxPackageFinalize(ClientData clientData) {

    UNUSED(clientData);

    Tcl_MutexLock(&vktcl_ctx2internal_mx);

    DBG2(printf("enter..."));

    if (vktcl_ctx_ht_initialized) {

        DBG2(printf("finalize"));

        Tcl_HashSearch search;
        Tcl_HashEntry* entry;
        for (entry = Tcl_FirstHashEntry(&vktcl_ctx2internal_ht, &search);
                entry != NULL;
                entry = Tcl_NextHashEntry(&search)) {
            vktcl_CtxClose((vktcl_CtxType *)Tcl_GetHashValue(entry));
        }

        Tcl_DeleteHashTable(&vktcl_ctx2internal_ht);

        vktcl_ctx_ht_initialized = 0;

    } else {
        DBG2(printf("not initialized"));
    }

    Tcl_MutexUnlock(&vktcl_ctx2internal_mx);
    Tcl_MutexFinalize(&vktcl_ctx2internal_mx);
    vktcl_ctx2internal_mx = NULL;

    DBG2(printf("ok"));

}

void vktcl_CtxPackageInitialize(void) {

    Tcl_MutexLock(&vktcl_ctx2internal_mx);

    DBG2(printf("enter..."));

    if (!vktcl_ctx_ht_initialized) {

        DBG2(printf("initialize"));

        Tcl_InitHashTable(&vktcl_ctx2internal_ht, TCL_STRING_KEYS);

        Tcl_CreateThreadExitHandler(vktcl_CtxPackageFinalize, NULL);

        vktcl_ctx_ht_initialized = 1;

    } else {
        DBG2(printf("already initialized"));
    }

    Tcl_MutexUnlock(&vktcl_ctx2internal_mx);

    DBG2(printf("ok"));

}
