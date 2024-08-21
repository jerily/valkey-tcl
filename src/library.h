/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#ifndef VALKEYTCL_LIBRARY_H
#define VALKEYTCL_LIBRARY_H

#include "common.h"

#ifdef ENABLE_SSL
#define VALKEYTCL_PKGCONFIG_ENABLE_SSL "1"
#else
#define VALKEYTCL_PKGCONFIG_ENABLE_SSL "0"
#endif /* ENABLE_SSL */

static Tcl_Config const tclvalkey_pkgconfig[] = {
    { "package-version", XSTR(VERSION) },
    { "feature-ssl",     VALKEYTCL_PKGCONFIG_ENABLE_SSL },
    {NULL, NULL}
};

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

#endif // VALKEYTCL_LIBRARY_H
