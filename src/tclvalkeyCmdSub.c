/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */

#include "tclvalkeyCmdSub.h"

#define DEFINE_SUBCMD(x) valkeyReply* vktcl_CtxHandleCmdSub##x(vktcl_CtxType *ctx, \
    Tcl_Interp *interp, int command_words, int objc, Tcl_Obj *const objv[])

DEFINE_SUBCMD(Universal) {

    UNUSED(interp);

    // Skip handle
    objc--;
    objv++;

    if (command_words == 1) {
        DBG2(printf("enter command [%s], have %d arguments",
            Tcl_GetString(objv[0]), objc - 1));
    } else {
        DBG2(printf("enter command [%s %s], have %d arguments",
            Tcl_GetString(objv[0]), Tcl_GetString(objv[1]), objc - 2));
    }

    const char **argv = ckalloc(sizeof(char *) * objc);
    size_t *argvlen = ckalloc(sizeof(size_t *) * objc);

    for (int i = 0; i < objc; i++) {
        Tcl_Size len;
        if (i < command_words) {
            DBG2(printf("add cmd#%d: [%s]", i, Tcl_GetString(objv[i])));
        } else {
            DBG2(printf("add arg#%d: [%s]", i - command_words, Tcl_GetString(objv[i])));
        }
        argv[i] = Tcl_GetStringFromObj(objv[i], &len);
        argvlen[i] = len;
    }

    DBG2(printf("call valkey command..."));
    valkeyReply *rc = valkeyCommandArgv(ctx->vk_ctx, objc, argv, argvlen);

    ckfree(argv);
    ckfree(argvlen);

    DBG2(printf("return: %s", (rc == NULL ? "<NULL>" : "OK")));
    return rc;


}

DEFINE_SUBCMD(Raw) {

    UNUSED(command_words);
    UNUSED(interp);

    // Skip command name and subcommand
    objc -= 2;
    objv += 2;

    DBG2(printf("enter, have %d arguments", objc));

    const char **argv = ckalloc(sizeof(char *) * objc);
    size_t *argvlen = ckalloc(sizeof(size_t *) * objc);

    for (int i = 0; i < objc; i++) {
        Tcl_Size len;
        DBG2(printf("add arg#%d: [%s]", i, Tcl_GetString(objv[i])));
        argv[i] = Tcl_GetStringFromObj(objv[i], &len);
        argvlen[i] = len;
    }

    DBG2(printf("call valkey command..."));
    valkeyReply *rc = valkeyCommandArgv(ctx->vk_ctx, objc, argv, argvlen);

    ckfree(argv);
    ckfree(argvlen);

    DBG2(printf("return: %s", (rc == NULL ? "<NULL>" : "OK")));
    return rc;

}

