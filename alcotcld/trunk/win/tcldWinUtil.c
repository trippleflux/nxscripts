/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Utilities

Author:
    neoxed (neoxed@gmail.com) Mar 13, 2006

Abstract:
    Windows specific utilities.

--*/

#include <tcld.h>

/*++

DebugPrint

    Prints a message to the debugger.

Arguments:
    format  - Pointer to a buffer containing a printf-style format string.

    ...     - Arguments to insert into 'format'.

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
    static char output[1024];
    va_list argList;

    va_start(argList, format);
    StringCchVPrintfA(output, ARRAYSIZE(output), format, argList);
    OutputDebugStringA(output);
    va_end(argList);
}
#endif
