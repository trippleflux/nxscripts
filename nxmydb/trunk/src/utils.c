/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous utilities.

*/

#include "mydb.h"

/*++

DebugPrint

    Prints a message to the debugger.

Arguments:
    format  - Pointer to a buffer containing a printf-style format string.

    ...     - Arguments to insert into "format".

Return Value:
    None.

--*/
#ifdef DEBUG
void
DebugPrint(
    const char *format,
    ...
    )
{
    char output[1024];
    va_list argList;

    va_start(argList, format);
    StringCchVPrintfA(output, ARRAYSIZE(output), format, argList);
    OutputDebugStringA(output);
    va_end(argList);
}
#endif
