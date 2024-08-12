/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */

#include "tclvalkeyCmd.h"
#include "tclvalkeyCmdSub.h"
#include "tclvalkeyCtx.h"
#include "tclvalkeyReply.h"

#include <sys/time.h> /* timeval struct */

static int vktcl_CtxHandleCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {

    int rc = TCL_OK;
    vktcl_CtxType *ctx = (vktcl_CtxType *)clientData;

    DBG2(printf("enter %p", (void *)ctx));

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "subcmd ?arg ...?");
        DBG2(printf("return: ERROR (wrong # args)"));
        return TCL_ERROR;
    }

    vktcl_CtxLock(ctx);

    if (!vktcl_CtxIsAlive(ctx)) {
        SetResult("valkey context is stalled");
        goto error;
    }

    if (ctx->vk_ctx->err) {
        SetResult("valkey context is in error state");
        goto error;
    }

    static const struct {
        const char *cmd;
        int isReply;
        vktcl_SubCmdProc *func;
    } subcommands[] = {
        {"raw",       1, vktcl_CtxHandleCmdSubRaw},
        {"configure", 0, vktcl_CtxHandleCmdSubConfigure},
        {NULL, NULL}
    };

    int idx;
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], subcommands, sizeof(subcommands[0]),
        "subcmd", 0, &idx) != TCL_OK)
    {
        goto error;
    }

    DBG2(printf("run subcmd [%s] ...", subcommands[idx].cmd));
    valkeyReply *reply = subcommands[idx].func(ctx, interp, objc, objv);

    // If subcommand returns raw return code (like the configure subcommand),
    // return it as is.
    if (!subcommands[idx].isReply) {
        rc = PTR2INT(reply);
        DBG2(printf("return: %s (raw response)", (rc == TCL_OK ? "OK" : "ERROR")));
        goto done;
    }

    // Check of error
    if (reply == NULL) {
        DBG2(printf("return: ERROR (valkey: %s)", ctx->vk_ctx->errstr));
        SetResult(ctx->vk_ctx->errstr);
        goto error;
    }

    Tcl_Obj *replyObj;
    rc = vktcl_ReplyToTclObject(reply, &replyObj, 0);
    Tcl_SetObjResult(interp, replyObj);

    freeReplyObject(reply);

    goto done;

error:
    rc = TCL_ERROR;

done:
    vktcl_CtxUnlock(ctx);
    return rc;

}

static void vktcl_CtxHandleDeleteProc(ClientData clientData) {
    vktcl_CtxType *ctx = (vktcl_CtxType *)clientData;
    DBG2(printf("delete command for context %p", (void *)ctx));
    ctx->cmdToken = NULL;
    vktcl_CtxClose(ctx);
    vktcl_CtxDecrRefCount(ctx);
}

static char *vktcl_CtxHandleVarTraceProc(ClientData clientData, Tcl_Interp *interp,
    const char *name1, const char *name2, int flags)
{

    vktcl_CtxType *ctx = (vktcl_CtxType *)clientData;

    if (!vktcl_CtxIsAlive(ctx)) {
        DBG2(printf("access to variable with stalled valkey context %p, stop tracing variable",
            (void *)ctx));
        if (!Tcl_InterpDeleted(interp)) {
            Tcl_UntraceVar(interp, name1, TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                vktcl_CtxHandleVarTraceProc, clientData);
        }
        vktcl_CtxDecrRefCount(ctx);
        return NULL;
    }

    if (flags & TCL_TRACE_WRITES) {
        DBG2(printf("return: error (write attempt on var [%s])", name1));
        // Restore value
        Tcl_SetVar2(interp, name1, name2, ctx->cmd, 0);
        return "readonly variable";
    }

    if (flags & TCL_TRACE_UNSETS) {
        DBG2(printf("close context %p", (void *)ctx));
        if (ctx->cmdToken != NULL) {
            Tcl_DeleteCommandFromToken(ctx->interp, ctx->cmdToken);
            // Context will be closed by the delete callback for the command
        }
        vktcl_CtxDecrRefCount(ctx);
    }

    return NULL;

}

static Tcl_Size copy_arg(void *clientData, Tcl_Interp *interp, Tcl_Size objc,
    Tcl_Obj *const *objv, void *dstPtr)
{
    if (objc < 1) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("\"%s\" option requires an"
            " additional argument", (char *)clientData));
        return -1;
    }
    *((Tcl_Obj **)dstPtr) = objv[0];
#if TCL_MAJOR_VERSION < 9
    return objc - 1;
#else
    return 1;
#endif
}

static int vktcl_CtxNewCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {

    UNUSED(clientData);

    const char *opt_host = NULL;
    int opt_port = 6379;
    int opt_ssl = 0;
    const char *opt_path = NULL;
    const char *opt_password = NULL;
    int opt_timeout = -1;
    Tcl_Obj *opt_var = NULL;

#pragma GCC diagnostic push
// ignore warning for copy_arg:
//     warning: ISO C forbids conversion of function pointer to object pointer type [-Wpedantic]
#pragma GCC diagnostic ignored "-Wpedantic"
    Tcl_ArgvInfo ArgTable[] = {
        { TCL_ARGV_STRING,   "-host",     NULL,             &opt_host,     "hostname to connect", NULL },
        { TCL_ARGV_INT,      "-port",     NULL,             &opt_port,     "post number to connect", NULL },
        { TCL_ARGV_CONSTANT, "-ssl",      INT2PTR(1),       &opt_ssl,      "use SSL/TLS for connection", NULL },
        { TCL_ARGV_STRING,   "-path",     NULL,             &opt_path,     "path to UNIX socket", NULL },
        { TCL_ARGV_STRING,   "-password", NULL,             &opt_password, "password for connection", NULL },
        { TCL_ARGV_INT,      "-timeout",  NULL,             &opt_timeout,  "timeout value in milliseconds for connecting and sending commands", NULL },
        { TCL_ARGV_GENFUNC,  "-var",      copy_arg,         &opt_var,      "name of the variable to be associated with the created valkey context", "-var" },
        TCL_ARGV_AUTO_HELP, TCL_ARGV_TABLE_END
    };
#pragma GCC diagnostic pop

    DBG2(printf("parse arguments"));

    if (Tcl_ParseArgsObjv(interp, ArgTable, &objc, objv, NULL) != TCL_OK) {
        DBG2(printf("return: ERROR (failed to parse args)"));
        return TCL_ERROR;
    }

    DBG2(printf("opt_host: [%s]", (opt_host == NULL ? "<NULL>" : opt_host)));
    DBG2(printf("opt_port: [%d]", opt_port));
    DBG2(printf("opt_ssl: %s", (opt_ssl ? "true" : "false")));
    DBG2(printf("opt_path: [%s]", (opt_path == NULL ? "<NULL>" : opt_path)));
    DBG2(printf("opt_password: [%s]", (opt_password == NULL ? "<NULL>" : "<hidden>")));
    DBG2(printf("opt_timeout: %d", opt_timeout));
    DBG2(printf("opt_var: [%s]", (opt_var == NULL ? "<NULL>" : Tcl_GetString(opt_var))));

    // Validate arguments

    if (opt_host == NULL && opt_path == NULL) {
        SetResult("-host or -path must be specified");
        DBG2(printf("return: ERROR (no -host or -path)"));
        return TCL_ERROR;
    }

    if (opt_host != NULL && opt_path != NULL) {
        SetResult("both -host and -path can not be specified");
        DBG2(printf("return: ERROR (both -host and -path)"));
        return TCL_ERROR;
    }

    if (opt_ssl && opt_host == NULL) {
        SetResult("-ssl can be only used when -host is specified");
        DBG2(printf("return: ERROR (-ssl without -host)"));
        return TCL_ERROR;
    }

    if (opt_port < 1 || opt_port > 65535) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("unsigned integer"
            " argument from 1 to 65535 is expected as -port option,"
            " but got \"%d\"", opt_port));
        DBG2(printf("return: ERROR (wrong -port [%d])", opt_port));
        return TCL_ERROR;
    }

    valkeyOptions options;
    memset(&options, 0, sizeof(valkeyOptions));
    struct timeval timeout_connect, timeout_command;

    if (opt_host != NULL) {
        VALKEY_OPTIONS_SET_TCP(&options, opt_host, opt_port);
    } else {
        VALKEY_OPTIONS_SET_UNIX(&options, opt_path);
    }

    if (opt_timeout >= 0) {

        timeout_connect.tv_sec = opt_timeout / 1000;
        timeout_connect.tv_usec = (opt_timeout % 1000) * 1000;
        options.connect_timeout = &timeout_connect;

        timeout_command.tv_sec = opt_timeout / 1000;
        timeout_command.tv_usec = (opt_timeout % 1000) * 1000;
        options.command_timeout = &timeout_command;

    }

    DBG2(printf("create valkey context"));

    valkeyContext *vk_ctx = valkeyConnectWithOptions(&options);
    if (vk_ctx == NULL) {
        SetResult("failed to allocate valkey context");
        DBG2(printf("return: ERROR (failed to alloc)"));
        return TCL_ERROR;
    }

    if (vk_ctx->err) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to connect: %s",
            vk_ctx->errstr));
        DBG2(printf("return: ERROR (failed to connect: %s)", vk_ctx->errstr));
        valkeyFree(vk_ctx);
        return TCL_ERROR;
    }

    vktcl_CtxType *ctx = vktcl_CtxNew(interp, vk_ctx);
    if (ctx == NULL) {
        SetResult("failed to add valkey context");
        DBG2(printf("return: ERROR (failed add context)"));
        valkeyFree(vk_ctx);
        return TCL_ERROR;
    }

    DBG2(printf("create command: %s", ctx->cmd));
    ctx->cmdToken = Tcl_CreateObjCommand(interp, ctx->cmd, vktcl_CtxHandleCmd,
        (ClientData)ctx, vktcl_CtxHandleDeleteProc);
    // Incr refcount for the command deletion callback
    vktcl_CtxIncrRefCount(ctx);

    Tcl_Obj *cmdObj = Tcl_NewStringObj(ctx->cmd, -1);

    if (opt_var != NULL) {
        DBG2(printf("bind var: %s", Tcl_GetString(opt_var)));
        Tcl_ObjSetVar2(interp, opt_var, NULL, cmdObj, 0);
        Tcl_TraceVar(interp, Tcl_GetString(opt_var),
            TCL_TRACE_WRITES | TCL_TRACE_UNSETS, vktcl_CtxHandleVarTraceProc,
            (ClientData)ctx);
        // Incr refcount for bound variable callback
        vktcl_CtxIncrRefCount(ctx);
    }

    Tcl_SetObjResult(interp, cmdObj);

    DBG2(printf("return: ok [%s]", ctx->cmd));
    return TCL_OK;

}

#if TCL_MAJOR_VERSION > 8
#define MIN_VERSION "9.0"
#else
#define MIN_VERSION "8.6"
#endif

int Tclvalkey_Init(Tcl_Interp *interp) {

    if (Tcl_InitStubs(interp, MIN_VERSION, 0) == NULL) {
        SetResult("Unable to initialize Tcl stubs");
        return TCL_ERROR;
    }

    vktcl_CtxPackageInitialize();
    Tcl_CreateNamespace(interp, "::valkey", NULL, NULL);
    Tcl_CreateObjCommand(interp, "valkey", vktcl_CtxNewCmd, NULL, NULL);

    return Tcl_PkgProvide(interp, "valkey", XSTR(VERSION));

}

#ifdef USE_NAVISERVER
int Ns_ModuleInit(const char *server, const char *module) {
    Ns_TclRegisterTrace(server, (Ns_TclTraceProc *) Tclvalkey_Init, server, NS_TCL_TRACE_CREATE);
    return NS_OK;
}
#endif
