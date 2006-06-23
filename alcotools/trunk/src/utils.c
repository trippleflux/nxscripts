/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements miscellaneous utilities.

--*/

#include "alcoholicz.h"

/*++

Panic

    Causes the application to panic and exit. This is usually invoked if the
    application encounters an unrecoverable error and must exit immediately.

Arguments:
    format  - Pointer to a buffer containing a printf-style format string.

    ...     - Arguments to insert into 'format'.

Return Value:
    None.

--*/
void
Panic(
    const char *format,
    ...
    )
{
    va_list argList;
    va_start(argList, format);

    vfprintf(stderr, format, argList);
#if (LOG_LEVEL > 0)
    LogFormatV(LOG_LEVEL_FATAL, format, argList);
#endif

    va_end(argList);
    ExitProcess(3);
}
