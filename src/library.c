/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */

#include "library.h"
#include "tclvalkeyCmdSub.h"
#include "tclvalkeyCtx.h"
#include "tclvalkeyReply.h"

#include <sys/time.h> /* timeval struct */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT // See MSDN Article Q83456
#endif
#include <windows.h> /* for Sleep() */
#undef WIN32_LEAN_AND_MEAN
#else
#include <unistd.h> /* for usleep() */
#endif /* _WIN32 */

typedef enum {
    VK_SUBCOMMAND_RETURNS_REPLY,
    VK_SUBCOMMAND_DESTROY
} vktcl_CommandType;

static const struct {
    const char *word1;
    const char *word2;
    int arg_num;
    vktcl_CommandType type;
    vktcl_SubCmdProc *func;
} vktcl_commands[] = {
    { "raw",       NULL, -2, VK_SUBCOMMAND_RETURNS_REPLY, vktcl_CtxHandleCmdSubRaw },
    { "destroy",   NULL,  1, VK_SUBCOMMAND_DESTROY,       NULL },
#define COMMAND(_type, _name, _subname, _arity, _keymethod, _keypos) \
    { _name, _subname, ( _subname == NULL ? _arity : (_arity < 0 ? (_arity + 1) : (_arity - 1))), VK_SUBCOMMAND_RETURNS_REPLY, vktcl_CtxHandleCmdSubUniversal },
#include "cmddef.h"
#undef COMMAND
    { NULL, NULL, 0, 0, NULL }
};

static int vktcl_ValidateCommand(Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {

    DBG2(printf("enter"));

    int idx;
    int i;
    Tcl_Obj *errmsg = NULL;
    int nargs;
    const char *arg_to_compare;

    // We are not using Tcl_GetIndexFromObjStruct() here because we have duplicates in
    // the command list and this function generates an error with those duplicates.
    arg_to_compare = Tcl_GetString(objv[1]);
    // This variable will be used to generate an error message.
    int command_count = 0;
    for (idx = 0; vktcl_commands[idx].word1 != NULL; idx++) {
        if (strcmp(vktcl_commands[idx].word1, arg_to_compare) == 0) {
            break;
        }
        command_count++;
    }

    // We could not find the command. Generate an error message.
    if (vktcl_commands[idx].word1 == NULL) {

        errmsg = Tcl_ObjPrintf("bad command \"%s\": must be ", arg_to_compare);
        int is_first = 1;
        command_count--;
        for (idx = 0; vktcl_commands[idx].word1 != NULL; idx++) {

            // Skip if the next command is the same as the current one
            if (idx < command_count && strcmp(vktcl_commands[idx].word1, vktcl_commands[idx + 1].word1) == 0) {
                continue;
            }

            // If we found the first command, just add it as is
            if (is_first) {
                Tcl_AppendToObj(errmsg, vktcl_commands[idx].word1, -1);
                is_first = 0;
                continue;
            }

            if (idx == command_count) {
                Tcl_AppendStringsToObj(errmsg, " or ", vktcl_commands[idx].word1, NULL);
            } else {
                Tcl_AppendStringsToObj(errmsg, ", ", vktcl_commands[idx].word1, NULL);
            }

        }

        goto error;

    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], vktcl_commands, sizeof(vktcl_commands[0]),
        "command", 0, &idx) != TCL_OK)
    {
        goto error;
    }

    DBG2(printf("the first command word is: [%s]", vktcl_commands[idx].word1));

    // Some commands contains 2 words. Let's check the second word.
    if (vktcl_commands[idx].word2 != NULL) {

        DBG2(printf("command has 2 words"));

        if (objc < 3) {

            // Let's give the user a clear error message indicating which
            // command requires the second word.
            errmsg = Tcl_ObjPrintf("%s subcommand ?arg ...?", vktcl_commands[idx].word1);
            Tcl_WrongNumArgs(interp, 1, objv, Tcl_GetString(errmsg));
            Tcl_BounceRefCount(errmsg);
            errmsg = NULL;

            goto error;

        }

        // We need to validate the second word. We'll go through the available
        // commands, compare the first word with the word we have already found,
        // and then check the second word.

        arg_to_compare = Tcl_GetString(objv[2]);
        // This variable contains the number of available subcommands for
        // a given command. It will be used to generate a nice error message
        // if we don't find the specified subcommand.
        int found_subcommand_count = 0;
        for (i = 0; vktcl_commands[i].word1 != NULL; i++) {

            // Skip if the first word doesn't match
            if (strcmp(vktcl_commands[i].word1, vktcl_commands[idx].word1) != 0) {
                continue;
            }

            // Check the second word
            if (strcmp(vktcl_commands[i].word2, arg_to_compare) == 0) {
                // We found the second word
                break;
            }

            found_subcommand_count++;

        }

        // If the second word is not found, then we stop at the NULL word.
        // Let's give user a clear error message with a list of available
        // subcommands.
        if (vktcl_commands[i].word1 == NULL) {

            errmsg = Tcl_ObjPrintf("unknown subcommand \"%s\" for command %s:"
                " must be ", arg_to_compare, vktcl_commands[idx].word1);
            // Go through the available commands again to collect a list of
            // known subcommands.
            int found_subcommand_number = 0;
            for (i = 0; vktcl_commands[i].word1 != NULL; i++) {

                if (strcmp(vktcl_commands[i].word1, vktcl_commands[idx].word1) != 0) {
                    continue;
                }

                found_subcommand_number++;

                if (found_subcommand_number == 1) {
                    Tcl_AppendToObj(errmsg, vktcl_commands[i].word2, -1);
                } else if (found_subcommand_number == found_subcommand_count) {
                    Tcl_AppendStringsToObj(errmsg, " or ", vktcl_commands[i].word2, NULL);
                } else {
                    Tcl_AppendStringsToObj(errmsg, ", ", vktcl_commands[i].word2, NULL);
                }

            }

            goto error;

        }

        // Let's use the found index
        idx = i;
        nargs = objc - 3;

    } else {
        nargs = objc - 2;
    }



    // Now, let's check number of arguments for the given command
    DBG2(printf("command wants %d args, have %d args", vktcl_commands[idx].arg_num, nargs));

    // Check if the commands takes no arguments
    if (vktcl_commands[idx].arg_num == 1) {

        if (nargs == 0) {
            // No arguments are specified.
            goto ok;
        }

        // We have some arguments that cannot be accepted.

        errmsg = Tcl_NewStringObj(vktcl_commands[idx].word1, -1);
        if (vktcl_commands[idx].word2 != NULL) {
            Tcl_AppendPrintfToObj(errmsg, " %s", vktcl_commands[idx].word2);
        }
        Tcl_AppendToObj(errmsg, " command doesn't accept arguments, but ", -1);
        if (nargs == 1) {
            Tcl_AppendToObj(errmsg, "1 argument was given", -1);
        } else {
            Tcl_AppendPrintfToObj(errmsg, "%d arguments were given", nargs);
        }

        goto error;

    }

    // Check if the command requires a minimum number of arguments
    if (vktcl_commands[idx].arg_num < 0) {

        if (nargs >= (abs(vktcl_commands[idx].arg_num) - 1)) {
            // We have a minimum number of arguments
            goto ok;
        }

        // We don't have enough arguments. Let's generate an appropriate error
        // message.

        errmsg = Tcl_NewStringObj(vktcl_commands[idx].word1, -1);
        if (vktcl_commands[idx].word2 != NULL) {
            Tcl_AppendPrintfToObj(errmsg, " %s", vktcl_commands[idx].word2);
        }
        if (vktcl_commands[idx].arg_num == -1) {
            Tcl_AppendToObj(errmsg, " command requires a minimum of 1 argument,"
                " but no arguments were given", -1);
        } else {
            Tcl_AppendPrintfToObj(errmsg, " command requires a minimum of %d arguments,"
                " but ", abs(vktcl_commands[idx].arg_num) - 1);
            if (nargs == 0) {
                Tcl_AppendPrintfToObj(errmsg, "no arguments were given");
            } else if (nargs == 1) {
                Tcl_AppendPrintfToObj(errmsg, "only 1 argument was given");
            } else {
                Tcl_AppendPrintfToObj(errmsg, "only %d arguments were given", nargs);
            }

        }

        goto error;

    }

    // Check if the command requires the exact number of arguments

    if (nargs == (vktcl_commands[idx].arg_num - 1)) {
        // We have the exact number of arguments
        goto ok;
    }

    // We don't have the exact number of arguments.
    errmsg = Tcl_NewStringObj(vktcl_commands[idx].word1, -1);
    if (vktcl_commands[idx].word2 != NULL) {
        Tcl_AppendPrintfToObj(errmsg, " %s", vktcl_commands[idx].word2);
    }
    Tcl_AppendToObj(errmsg, " command requires exactly ", -1);
    if (vktcl_commands[idx].arg_num == 2) {
        Tcl_AppendToObj(errmsg, "1 argument, but ", -1);
    } else {
        Tcl_AppendPrintfToObj(errmsg, "%d arguments, but ", (vktcl_commands[idx].arg_num - 1));
    }
    if (nargs == 0) {
        Tcl_AppendToObj(errmsg, "no arguments were given", -1);
    } else if (nargs == 1) {
        Tcl_AppendToObj(errmsg, "only 1 argument was given", -1);
    } else {
        Tcl_AppendPrintfToObj(errmsg, "%d arguments were given", nargs);
    }

error:

    if (errmsg != NULL) {
        Tcl_SetObjResult(interp, errmsg);
    }

    DBG2(printf("return: ERROR (%s)", Tcl_GetStringResult(interp)));

    return -1;

ok:

   DBG2(printf("return: ok (%d)", idx));
   return idx;

}

static int vktcl_CtxHandleCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {

    int rc = TCL_OK;
    vktcl_CtxType *ctx = (vktcl_CtxType *)clientData;

    DBG2(printf("enter %p", (void *)ctx));

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "command ?subcommand? ?arg ...?");
        DBG2(printf("return: ERROR (wrong # args)"));
        return TCL_ERROR;
    }

    vktcl_CtxLock(ctx);

    if (!vktcl_CtxIsAlive(ctx)) {
        SetResult("valkey context is stalled");
        goto error;
    }

    int idx = vktcl_ValidateCommand(interp, objc, objv);
    if (idx < 0) {
        goto error;
    }

    // Handle the destroy subcommand in special way
    if (vktcl_commands[idx].type == VK_SUBCOMMAND_DESTROY) {
        // If destroyed, unlock the context and delete the command handler.
        // Everything will be freed when the command handler is released.
        // Return the name of our deleted command, just to return at least
        // something.
        SetResult(ctx->cmd);
        vktcl_CtxUnlock(ctx);
        Tcl_DeleteCommandFromToken(ctx->interp, ctx->cmdToken);
        return TCL_OK;
    }

    // Do not allow commands to be executed on a context in an error state.
    // Verify that the current command is a command for the remote server by
    // checking vktcl_commands[idx].type. We should allow other commands
    // to pass even if context is in error state.
    if (ctx->vk_ctx->err && vktcl_commands[idx].type == VK_SUBCOMMAND_RETURNS_REPLY) {
        SetResult("valkey context is in error state");
        goto error;
    }

    int command_words = (vktcl_commands[idx].word2 == NULL ? 1 : 2);
    int retry = 0;
    int retry_delay = 0;
    valkeyReply *reply;

retryCommand:

    reply = vktcl_commands[idx].func(ctx, interp, command_words, objc, objv);

    // Check of error
    if (reply == NULL) {
        // If valkey context is in error state, then return error from the context.
        // Otherwise, we will get an en error from the subcommand, and it should
        // set the appropriate error message to the interp result.
        if (ctx->vk_ctx->err) {

retryConnect:

            if (retry >= ctx->retryCount) {
                DBG2(printf("number of retries exceeded, retry: %d, maximum: %d",
                    retry, ctx->retryCount));
                goto skipRetry;
            }
            retry++;

            if (retry_delay) {
                DBG2(printf("reconnect #%d after %d milliseconds", retry, retry_delay));
#ifdef _WIN32
                // Sleep is in milliseconds
                Sleep(retry_delay);
#else
                // usleep is in microseconds
                usleep(retry_delay * 1000);
#endif /* _WIN32 */
            } else {
                DBG2(printf("reconnect #%d without delay", retry));
            }
            retry_delay = (retry_delay + 1) * 2;

            if (valkeyReconnect(ctx->vk_ctx) == VALKEY_OK) {
                DBG2(printf("reconnection was successful"));
                goto retryCommand;
            } else {
                DBG2(printf("reconnection failed"));
                goto retryConnect;
            }

skipRetry:

            DBG2(printf("return: ERROR (valkey: %s)", ctx->vk_ctx->errstr));
            SetResult(ctx->vk_ctx->errstr);
        } else {
            DBG2(printf("return: ERROR (subcmd: %s)", Tcl_GetStringResult(interp)));
        }
        goto error;
    }

    Tcl_Obj *replyObj;
    rc = vktcl_ReplyToTclObject(reply, &replyObj, ctx->isReplyTyped);
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

static int vktcl_CheckAuth(Tcl_Interp *interp, valkeyContext *vk_ctx, int *isAuthRequired) {

    int rc = TCL_OK;

    DBG2(printf("send command: PING..."));

    valkeyReply *reply = valkeyCommand(vk_ctx, "PING");

    if (reply == NULL) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("error when checking valkey server"
            " authentication status: %s", vk_ctx->errstr));
        DBG2(printf("return: ERROR (got NULL)"));
        return TCL_ERROR;
    }

    // If server requires authentication, it returns reply with:
    //     type: VALKEY_REPLY_ERROR
    //     str: NOAUTH Authentication required.
    if (reply->type == VALKEY_REPLY_ERROR) {

        DBG2(printf("got expected reply (error)"));

        // Make sure that we have the expected error message
        if (strncmp(reply->str, "NOAUTH ", 7) == 0) {
            // We are good. Server requires authentication.
            DBG2(printf("return: OK (auth required)"));
            *isAuthRequired = 1;
            goto done;
        }

        // We got unexpected error
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("got an unexpected error when"
            " checking whether the valkey server requires authentication: %s",
            reply->str));
        DBG2(printf("return: ERROR (unexpected error reply: %s)", reply->str));
        goto error;

    }

    // If we are here, then server returned not an error. We only expect
    // a PONG response. Make sure that we have expected response.
    if (reply->type == VALKEY_REPLY_STATUS) {

        DBG2(printf("got expected reply (status)"));

        // Make sure that we have expected message
        if (strcmp(reply->str, "PONG") == 0) {
            // We are good. Server does not require authentication.
            DBG2(printf("return: OK (auth not required)"));
            *isAuthRequired = 0;
            goto done;
        }

        // We got unexpected message
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("got an unexpected message when"
            " checking whether the valkey server requires authentication: %s",
            reply->str));
        DBG2(printf("return: ERROR (unexpected status reply: %s)", reply->str));
        goto error;

    }

    // Let's complain about unexpected reply
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("got an unexpected reply type when"
        " checking whether the valkey server requires authentication: %d",
        (int)reply->type));
    DBG2(printf("return: ERROR (unexpected reply type: %d)", (int)reply->type));
    goto error;

error:

    rc = TCL_ERROR;

done:

    freeReplyObject(reply);
    return rc;

}

// This function uses Tcl_ParseArgsObjv to parse arguments. However,
// Tcl_ParseArgsObjv has the disadvantage that we cannot get the argument
// value as a Tcl object.
//
// We can use TCL_ARGV_GENFUNC here. But this argument type is broken in
// tcl8.6.14. It allows to redefine objc, but it doesn't allow to
// increase srcIndex:
//     https://github.com/tcltk/tcl/blob/ebd3331e6c2852474d5c953d3058d2677e766f4d/generic/tclIndexObj.c#L1239-L1244
// In Tcl9 this argument type works. But we need to be compatible with Tcl8
// as well.
//
// We can use TCL_ARGV_FUNC here. But this argument type does not allow us
// to return an error if the value is missing.
//
// So here we'll use TCL_ARGV_FUNC, but set output to 1 if it doesn't have
// a value. After Tcl_ParseArgsObjv, we will check if the variables contain
// this value and return an appropriate error message.

static int copy_arg(void *clientData, Tcl_Obj *objPtr, void *dstPtr) {
    UNUSED(clientData);
    if (objPtr == NULL) {
        *((void **)dstPtr) = INT2PTR(1);
    } else {
        *((Tcl_Obj **)dstPtr) = objPtr;
    }
    return 1;
}

static int vktcl_CtxNewCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {

    UNUSED(clientData);

    const char *opt_host = NULL;
    int opt_port = 6379;
    int opt_ssl = 0;
    const char *opt_ssl_ca_file = NULL;
    const char *opt_ssl_cert_file = NULL;
    const char *opt_ssl_key_file = NULL;
    const char *opt_path = NULL;
    Tcl_Obj *opt_password = NULL;
    const char *opt_username = NULL;
    int opt_timeout = -1;
    Tcl_Obj *opt_var = NULL;
    int opt_retry_count = 5;
    int opt_reply_typed = 0;

#pragma GCC diagnostic push
// ignore warning for copy_arg:
//     warning: ISO C forbids conversion of function pointer to object pointer type [-Wpedantic]
#pragma GCC diagnostic ignored "-Wpedantic"
    Tcl_ArgvInfo ArgTable[] = {
        { TCL_ARGV_STRING,   "-path",          NULL,       &opt_path,          "path to UNIX socket", NULL },
        { TCL_ARGV_STRING,   "-host",          NULL,       &opt_host,          "hostname to connect", NULL },
        { TCL_ARGV_INT,      "-port",          NULL,       &opt_port,          "post number to connect", NULL },
        { TCL_ARGV_CONSTANT, "-ssl",           INT2PTR(1), &opt_ssl,           "use SSL/TLS for connection", NULL },
        { TCL_ARGV_STRING,   "-ssl_ca_file",   NULL,       &opt_ssl_ca_file,   "path to a CA certificate/bundle", NULL },
        { TCL_ARGV_STRING,   "-ssl_cert_file", NULL,       &opt_ssl_cert_file, "path to a client SSL certificate", NULL },
        { TCL_ARGV_STRING,   "-ssl_key_file",  NULL,       &opt_ssl_key_file,  "path to a key to the client SSL certificate", NULL },
        { TCL_ARGV_FUNC,     "-password",      copy_arg,   &opt_password,      "password for authentication", NULL },
        { TCL_ARGV_STRING,   "-username",      NULL,       &opt_username,      "username for authentication", NULL },
        { TCL_ARGV_INT,      "-timeout",       NULL,       &opt_timeout,       "timeout value in milliseconds for connecting and sending commands", NULL },
        { TCL_ARGV_FUNC,     "-var",           copy_arg,   &opt_var,           "name of the variable to be associated with the created valkey context", NULL },
        { TCL_ARGV_INT,      "-retry_count",   NULL,       &opt_retry_count,   "number of attempts to send a command in case of connection issues", NULL },
        { TCL_ARGV_CONSTANT, "-reply_typed",   INT2PTR(1), &opt_reply_typed,   "return a reply type along with a message", NULL },
        TCL_ARGV_AUTO_HELP, TCL_ARGV_TABLE_END
    };
#pragma GCC diagnostic pop

    DBG2(printf("parse arguments"));

    Tcl_Size temp_objc = objc;
    if (Tcl_ParseArgsObjv(interp, ArgTable, &temp_objc, objv, NULL) != TCL_OK) {
        DBG2(printf("return: ERROR (failed to parse args)"));
        return TCL_ERROR;
    }

    if (opt_var == INT2PTR(1)) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("\"%s\" option requires an"
            " additional argument", "-var"));
        DBG2(printf("return: ERROR (missing value for -var)"));
        return TCL_ERROR;
    }

    if (opt_password == INT2PTR(1)) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("\"%s\" option requires an"
            " additional argument", "-password"));
        DBG2(printf("return: ERROR (missing value for -password)"));
        return TCL_ERROR;
    }

    DBG2(printf("opt_host: [%s]", (opt_host == NULL ? "<NULL>" : opt_host)));
    DBG2(printf("opt_port: [%d]", opt_port));
    DBG2(printf("opt_ssl: %s", (opt_ssl ? "true" : "false")));
    DBG2(printf("opt_ssl_ca_file: %s", (opt_ssl_ca_file == NULL ? "<NULL>" : opt_ssl_ca_file)));
    DBG2(printf("opt_ssl_cert_file: %s", (opt_ssl_cert_file == NULL ? "<NULL>" : opt_ssl_cert_file)));
    DBG2(printf("opt_ssl_key_file: %s", (opt_ssl_key_file == NULL ? "<NULL>" : opt_ssl_key_file)));
    DBG2(printf("opt_path: [%s]", (opt_path == NULL ? "<NULL>" : opt_path)));
    DBG2(printf("opt_password: [%s]", (opt_password == NULL ? "<NULL>" : "<hidden>")));
    DBG2(printf("opt_timeout: %d", opt_timeout));
    DBG2(printf("opt_var: [%s]", (opt_var == NULL ? "<NULL>" : Tcl_GetString(opt_var))));
    DBG2(printf("opt_retry_count: %d", opt_retry_count));
    DBG2(printf("opt_reply_typed: %s", (opt_reply_typed ? "true" : "false")));

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

#ifndef ENABLE_SSL
    if (opt_ssl || opt_ssl_ca_file != NULL) {
        SetResult("this package was built without SSL support");
        DBG2(printf("return: ERROR (no ssl support)"));
        return TCL_ERROR;
    }
#endif /* ENABLE_SSL */

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

    if (opt_password == NULL && opt_username != NULL) {
        SetResult("-username can be only used when -password is specified");
        DBG2(printf("return: ERROR (-username without -password)"));
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

#ifdef ENABLE_SSL
    valkeySSLContext *ssl = NULL;
#endif /* ENABLE_SSL */

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
        goto error;
    }

#ifdef ENABLE_SSL
    if (opt_ssl) {

        DBG2(printf("initialize SSL context"));

        valkeySSLContextError ssl_error = VALKEY_SSL_CTX_NONE;
        ssl = valkeyCreateSSLContext(opt_ssl_ca_file, NULL, opt_ssl_cert_file,
            opt_ssl_key_file, NULL, &ssl_error);

        if(ssl == NULL || ssl_error != VALKEY_SSL_CTX_NONE) {
            Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to initialize SSL: %s",
                (ssl_error != VALKEY_SSL_CTX_NONE ? valkeySSLContextGetError(ssl_error) : "Unknown error")));
            DBG2(printf("return: ERROR (ssl ctx failed: %s)",
                (ssl_error != VALKEY_SSL_CTX_NONE ? valkeySSLContextGetError(ssl_error) : "Unknown error")));
            goto error;
        }

        DBG2(printf("initialize valkey SSL context"));

        if (valkeyInitiateSSLWithContext(vk_ctx, ssl) != VALKEY_OK) {
            Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create SSL channel: %s",
                vk_ctx->errstr));
            DBG2(printf("return: ERROR (ssl channel failed: %s)", vk_ctx->errstr));
            goto error;
        }

    }
#endif /* ENABLE_SSL */


    int isAuthRequired;
    if (vktcl_CheckAuth(interp, vk_ctx, &isAuthRequired) != TCL_OK) {
        goto error;
    }

    if (opt_password == NULL) {

        if (isAuthRequired) {
            SetResult("valkey server requires authentication, but no password is provided");
            DBG2(printf("return: ERROR (no password)"));
            goto error;
        }

        DBG2(printf("password is not specified and server accepts connections without a password"));

    } else {

        valkeyReply *reply;
        Tcl_Size passwordLen;
        const char *passwordStr = Tcl_GetStringFromObj(opt_password, &passwordLen);

        if (opt_username == NULL) {
            DBG2(printf("send auth command..."));
            reply = valkeyCommand(vk_ctx, "AUTH %b", passwordStr, (size_t)passwordLen);
        } else {
            DBG2(printf("send auth command (user: %s) ...", opt_username));
            reply = valkeyCommand(vk_ctx, "AUTH %s %b", opt_username, passwordStr, (size_t)passwordLen);
        }

        if (reply == NULL) {
            SetResult("error when authenticating on the valkey server: null reply");
            DBG2(printf("return: ERROR (NULL reply)"));
            goto error;
        }

        if (reply->type != VALKEY_REPLY_STATUS || strcmp(reply->str, "OK") != 0) {

            Tcl_Obj *replyObj = NULL;
            vktcl_ReplyToTclObject(reply, &replyObj, 0);

            if (replyObj == NULL) {
                replyObj = Tcl_NewStringObj("unknown reply", -1);
            }

            Tcl_SetObjResult(interp, Tcl_ObjPrintf("error when authenticating on"
                " the valkey server: %s (user: %s)", Tcl_GetString(replyObj),
                (opt_username == NULL ? "default" : opt_username)));
            DBG2(printf("return: ERROR (%s)", Tcl_GetString(replyObj)));
            Tcl_BounceRefCount(replyObj);
            freeReplyObject(reply);
            goto error;

        }

        DBG2(printf("authorization was successful"));
        freeReplyObject(reply);

    }

    vktcl_CtxType *ctx = vktcl_CtxNew(interp, vk_ctx);
    if (ctx == NULL) {
        SetResult("failed to add valkey context");
        DBG2(printf("return: ERROR (failed add context)"));
        goto error;
    }

    ctx->retryCount = opt_retry_count;
    ctx->isReplyTyped = opt_reply_typed;
#ifdef ENABLE_SSL
    ctx->ssl = ssl;
#endif /* ENABLE_SSL */

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

error:

    valkeyFree(vk_ctx);
#ifdef ENABLE_SSL
    if (ssl != NULL) {
        valkeyFreeSSLContext(ssl);
    }
#endif /* ENABLE_SSL */
    return TCL_ERROR;

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

    Tcl_RegisterConfig(interp, "valkey", tclvalkey_pkgconfig, "iso8859-1");

    return Tcl_PkgProvide(interp, "valkey", XSTR(VERSION));

}

#ifdef USE_NAVISERVER
int Ns_ModuleInit(const char *server, const char *module) {
    Ns_TclRegisterTrace(server, (Ns_TclTraceProc *) Tclvalkey_Init, server, NS_TCL_TRACE_CREATE);
    return NS_OK;
}
#endif
