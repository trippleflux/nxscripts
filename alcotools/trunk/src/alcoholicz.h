/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Common

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Common header file that includes all necessary headers.

--*/

#ifndef _ALCOHOLICZ_H_
#define _ALCOHOLICZ_H_

//
// Alcohol status codes
//

typedef enum {
    ALCOHOL_OK = 0,
    ALCOHOL_ERROR,
    ALCOHOL_INSUFFICIENT_BUFFER,
    ALCOHOL_INSUFFICIENT_MEMORY,
    ALCOHOL_INVALID_DATA,
    ALCOHOL_INVALID_PARAMETER,
    ALCOHOL_UNKNOWN
} ALCOHOL;

//
// Common includes
//

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include "buildopts.h"
#include "platform.h"

#if !defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN)
#   error "Host byte-order is unknown, define BIG_ENDIAN or LITTLE_ENDIAN."
#endif

// Third-party libraries
#include "queue.h"
#include "sqlite3.h"
#include "zlib.h"

// Functions and subsystems
#include "alloc.h"
#include "cfgread.h"
#include "crc32.h"
#include "cstring.h"
#include "dynstring.h"
#include "events.h"
#include "logging.h"
#include "template.h"
#include "utils.h"

#endif // _ALCOHOLICZ_H_
