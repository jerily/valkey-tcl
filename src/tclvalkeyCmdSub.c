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

DEFINE_SUBCMD(Configure) {

    UNUSED(command_words);

    DBG2(printf("enter, have %d arguments", objc));

    static const char *const options[] = {
        "-blocking", "-reply_typed",
        NULL
    };

    enum options {
        optBlocking, optReplyTyped,
        optionsCount
    };

    int opt;

    for (int arg_idx = 2; arg_idx < objc; arg_idx++) {

        if (Tcl_GetIndexFromObj(interp, objv[arg_idx], options, "option", 0,
            &opt) != TCL_OK)
        {
            goto error;
        }

        DBG2(printf("specified option: %s", options[opt]));

        if (++arg_idx >= objc) {
            Tcl_SetObjResult(interp, Tcl_ObjPrintf("missing value for"
                " option \"%s\"", options[opt]));
            goto error;
        }

        // As for now, we have only boolean options

        int val;
        if (Tcl_GetBooleanFromObj(NULL, objv[arg_idx], &val) != TCL_OK) {
            Tcl_SetObjResult(interp, Tcl_ObjPrintf("expected a boolean value"
                " for option \"%s\", but got \"%s\"", options[opt],
                Tcl_GetString(objv[arg_idx])));
            goto error;
        }

        switch ((enum options)opt) {
        case optBlocking:
            ctx->isBlocking = val;
            break;
        case optReplyTyped:
            ctx->isReplyTyped = val;
            break;
        case optionsCount:
            break;
        }

    }

    // Return a dict with all options

    Tcl_Obj *rc = Tcl_NewDictObj();

    for (opt = 0; opt < optionsCount; opt++) {

        Tcl_Obj *key = Tcl_NewStringObj(options[opt], -1);
        Tcl_Obj *val;

        switch ((enum options)opt) {
        case optBlocking:
            val = Tcl_NewBooleanObj(ctx->isBlocking);
            break;
        case optReplyTyped:
            val = Tcl_NewBooleanObj(ctx->isReplyTyped);
            break;
        case optionsCount:
            break;
        }

        Tcl_DictObjPut(NULL, rc, key, val);

    }

    Tcl_SetObjResult(interp, rc);

    DBG2(printf("return: ok"));
    return INT2PTR(TCL_OK);

error:
    DBG2(printf("return: ERROR"));
    return INT2PTR(TCL_ERROR);

}
