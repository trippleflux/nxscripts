/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Platform

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Windows specific includes and definitions.

--*/

#ifndef _PLATWIN_H_
#define _PLATWIN_H_

#define LITTLE_ENDIAN 1
#define WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if !defined(__cplusplus)
#   undef inline
#   if (_MSC_VER >= 1200)
#       define inline __forceinline
#   elif defined(_MSC_VER)
#       define inline __inline
#   else
#       define inline
#   endif
#endif // inline

#endif // _PLATWIN_H_
