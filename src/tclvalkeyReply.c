/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */

#include "tclvalkeyReply.h"

static Tcl_Obj *vktcl_ReplyGetType(valkeyReply *reply) {

    // Special case for VERB reply
    if (reply->type == VALKEY_REPLY_VERB) {
        return Tcl_ObjPrintf("VERB:%s", reply->vtype);
    }

    const char *res;

    switch(reply->type) {
    case VALKEY_REPLY_INTEGER:
        res = "INTEGER";
        break;
    case VALKEY_REPLY_NIL:
        res = "NIL";
        break;
    case VALKEY_REPLY_BOOL:
        res = "BOOL";
        break;
    case VALKEY_REPLY_PUSH:
        res = "PUSH";
        break;
    case VALKEY_REPLY_ARRAY:
        res = "ARRAY";
        break;
    case VALKEY_REPLY_SET:
        res = "SET";
        break;
    case VALKEY_REPLY_MAP:
        res = "MAP";
        break;
    case VALKEY_REPLY_ATTR:
        res = "ATTR";
        break;
    case VALKEY_REPLY_ERROR:
        res = "ERROR";
        break;
    case VALKEY_REPLY_STATUS:
        res = "STATUS";
        break;
    case VALKEY_REPLY_STRING:
        res = "STRING";
        break;
    case VALKEY_REPLY_DOUBLE:
        res = "DOUBLE";
        break;
    case VALKEY_REPLY_BIGNUM:
        res = "BIGNUM";
        break;
    case VALKEY_REPLY_VERB:
        break;
    default:
        res = "UNKNOWN";
        break;
    }

    return Tcl_NewStringObj(res, -1);

}

#define DBG_ENTER(x) DBG2(printf("enter, reply type: %s", \
        ((x)->type == VALKEY_REPLY_STATUS  ? "VALKEY_REPLY_STATUS"  : \
        ((x)->type == VALKEY_REPLY_ERROR   ? "VALKEY_REPLY_ERROR"   : \
        ((x)->type == VALKEY_REPLY_INTEGER ? "VALKEY_REPLY_INTEGER" : \
        ((x)->type == VALKEY_REPLY_NIL     ? "VALKEY_REPLY_NIL"     : \
        ((x)->type == VALKEY_REPLY_STRING  ? "VALKEY_REPLY_STRING"  : \
        ((x)->type == VALKEY_REPLY_ARRAY   ? "VALKEY_REPLY_ARRAY"   : \
        ((x)->type == VALKEY_REPLY_DOUBLE  ? "VALKEY_REPLY_DOUBLE"  : \
        ((x)->type == VALKEY_REPLY_BOOL    ? "VALKEY_REPLY_BOOL"    : \
        ((x)->type == VALKEY_REPLY_MAP     ? "VALKEY_REPLY_MAP"     : \
        ((x)->type == VALKEY_REPLY_SET     ? "VALKEY_REPLY_SET"     : \
        ((x)->type == VALKEY_REPLY_PUSH    ? "VALKEY_REPLY_PUSH"    : \
        ((x)->type == VALKEY_REPLY_ATTR    ? "VALKEY_REPLY_ATTR"    : \
        ((x)->type == VALKEY_REPLY_BIGNUM  ? "VALKEY_REPLY_BIGNUM"  : \
        ((x)->type == VALKEY_REPLY_VERB    ? "VALKEY_REPLY_VERB"    : \
            "UNKNOWN"))))))))))))))))

int vktcl_ReplyToTclObject(valkeyReply *reply, Tcl_Obj **obj, int isTyped) {

    DBG_ENTER(reply);

    int rc = TCL_OK;

    Tcl_Obj *res;

    switch(reply->type) {
    case VALKEY_REPLY_INTEGER:
        res = Tcl_NewWideIntObj(reply->integer);
        break;
    case VALKEY_REPLY_NIL:
        res = Tcl_NewObj();
        break;
    case VALKEY_REPLY_BOOL:
        res = Tcl_NewBooleanObj(reply->integer);
        break;
    case VALKEY_REPLY_PUSH:
    case VALKEY_REPLY_ARRAY:
    case VALKEY_REPLY_SET:
        res = Tcl_NewListObj(0, NULL);
        if (reply->element != NULL) {
            for (size_t i = 0; i < reply->elements; i++) {
                Tcl_Obj *elementObj;
                vktcl_ReplyToTclObject(reply->element[i], &elementObj, isTyped);
                Tcl_ListObjAppendElement(NULL, res, elementObj);
            }
        }
        break;
    case VALKEY_REPLY_MAP:
    case VALKEY_REPLY_ATTR:
        res = Tcl_NewDictObj();
        if (reply->element != NULL) {
            for (size_t i = 0; i < reply->elements; i++) {
                Tcl_Obj *keyObj, *valObj;
                vktcl_ReplyToTclObject(reply->element[i], &keyObj, isTyped);
                vktcl_ReplyToTclObject(reply->element[++i], &valObj, isTyped);
                Tcl_DictObjPut(NULL, res, keyObj, valObj);
            }
        }
        break;
    case VALKEY_REPLY_ERROR:
        res = Tcl_NewStringObj(reply->str, reply->len);
        rc = TCL_ERROR;
        break;
    case VALKEY_REPLY_STATUS:
    case VALKEY_REPLY_STRING:
    case VALKEY_REPLY_DOUBLE:
    case VALKEY_REPLY_BIGNUM:
        res = Tcl_NewStringObj(reply->str, reply->len);
        break;
    case VALKEY_REPLY_VERB:
        res = Tcl_NewByteArrayObj((const unsigned char *)reply->str, reply->len);
        break;
    default:
        res = Tcl_NewStringObj("unknown reply type", -1);
        rc = TCL_ERROR;
        break;
    }

    if (!isTyped) {
        DBG2(printf("return simple: %s: [%s]", (rc == TCL_OK ? "OK" : "ERROR"),
            Tcl_GetString(res)));
        *obj = res;
        return rc;
    }

    *obj = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(NULL, *obj, vktcl_ReplyGetType(reply));
    Tcl_ListObjAppendElement(NULL, *obj, res);

    DBG2(printf("return typed: %s: [%s]", (rc == TCL_OK ? "OK" : "ERROR"),
        Tcl_GetString(*obj)));

    return rc;

}
