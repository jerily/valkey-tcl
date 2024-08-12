/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#ifndef VALKEYTCL_TCLVALKEYCMD_H
#define VALKEYTCL_TCLVALKEYCMD_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

EXTERN int Tclvalkey_Init(Tcl_Interp *interp);
#ifdef USE_NAVISERVER
NS_EXTERN int Ns_ModuleVersion = 1;
NS_EXTERN int Ns_ModuleInit(const char *server, const char *module);
#endif

#ifdef __cplusplus
}
#endif

#endif // VALKEYTCL_TCLVALKEYCMD_H
