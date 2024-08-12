/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */

#include "tclvalkeyCmdSub.h"

#define DEFINE_SUBCMD(x) valkeyReply* vktcl_CtxHandleCmdSub##x(vktcl_CtxType *ctx, \
    Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])

DEFINE_SUBCMD(Raw) {

    DBG2(printf("enter, have %d arguments", objc));

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "valkeycmd ?arg ...?");
        return NULL;
    }

    // Skip command name and subcommand
    objc -= 2;
    objv += 2;

    const char **argv = ckalloc(sizeof(char *) * objc);
    size_t *argvlen = ckalloc(sizeof(size_t *) * objc);

    for (int i = 0; i < objc; i++) {
        Tcl_Size len;
        DBG2(printf("add arg#%d: [%s]", i, Tcl_GetString(objv[i])));
        argv[i] = Tcl_GetStringFromObj(objv[i], &len);
        argvlen[i] = len;
    }

    valkeyReply *rc = valkeyCommandArgv(ctx->vk_ctx, objc, argv, argvlen);

    ckfree(argv);
    ckfree(argvlen);

    return rc;

}

DEFINE_SUBCMD(Configure) {
    UNUSED(ctx);
    UNUSED(interp);
    UNUSED(objc);
    UNUSED(objv);
    return INT2PTR(TCL_OK);
}
