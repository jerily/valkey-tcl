/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#ifndef VALKEYTCL_TCLVALKEYREPLY_H
#define VALKEYTCL_TCLVALKEYREPLY_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

int vktcl_ReplyToTclObject(valkeyReply *reply, Tcl_Obj **obj, int isTyped);

#ifdef __cplusplus
}
#endif

#endif // VALKEYTCL_TCLVALKEYREPLY_H
