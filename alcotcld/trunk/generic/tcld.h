/*
 * AlcoTcld - Alcoholicz Tcl daemon.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   tcld.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) July 17, 2005
 *
 * Abstract:
 *   Common include file.
 */

#ifndef _TCLD_H_
#define _TCLD_H_

#include <tcl.h>

#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
#   include "../win/tcldWin.h"
#else
#   include "../unix/tcldUnix.h"
#endif

/* ARRAYSIZE - Returns the number of entries in an array. */
#ifdef ARRAYSIZE
#   undef ARRAYSIZE
#endif
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

#endif /* _TCLD_H_ */
