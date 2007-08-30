/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    DLL Entry

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point for dynamic libraries.

*/

#include <base.h>

/*

DllMain

    DLL entry point.

Arguments:
    instance - Handle to the DLL module.

    reason   - Reason the entry point is being called.

    reserved - Not used.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

*/
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(reserved);

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            TRACE_HEAD();
            break;
        case DLL_PROCESS_DETACH:
            TRACE_FOOT();
            break;
    }
    return TRUE;
}
