/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Build Options

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Compilation options.

--*/

#ifndef _BUILDOPTS_H_
#define _BUILDOPTS_H_

//
// DEBUG_ASSERT <TRUE/FALSE>
//  - Assertions to validate internal code.
//
// DEBUG_MEMORY <TRUE/FALSE>
//  - Track memory allocations to ensure resources are freed properly.
//
// LOG_LEVEL    <0-4>
//  - Compile-time log verbosity level.
//
#ifdef DEBUG
#   ifndef DEBUG_ASSERT
#       define DEBUG_ASSERT TRUE
#   endif
#   ifndef DEBUG_MEMORY
#       define DEBUG_MEMORY TRUE
#   endif
#   ifndef LOG_LEVEL
#       define LOG_LEVEL    4
#   endif
#else
#   ifndef DEBUG_ASSERT
#       define DEBUG_ASSERT FALSE
#   endif
#   ifndef DEBUG_MEMORY
#       define DEBUG_MEMORY FALSE
#   endif
#   ifndef LOG_LEVEL
#       define LOG_LEVEL    3
#   endif
#endif // DEBUG

//
// CONFIG_FILE  <string>
//  - Configuration file name.
//
#ifndef CONFIG_FILE
#   define CONFIG_FILE      "AlcoTools.conf"
#endif

//
// LOG_FILE     <string>
//  - Log file name.
//
#ifndef LOG_FILE
#   define LOG_FILE         "AlcoTools.log"
#endif

#endif // _BUILDOPTS_H_
