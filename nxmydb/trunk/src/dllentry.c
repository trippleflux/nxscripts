/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    DLL Entry

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point for dynamic libraries.

*/

#include "mydb.h"

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
BOOL
WINAPI
DllMain(
    HINSTANCE instance,
    DWORD reason,
    LPVOID reserved
    )
{
    UnreferencedParameter(instance);
    UnreferencedParameter(reserved);

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DebugHead();
#ifdef _MD
            // The static CRT requires thread notifications.
            DisableThreadLibraryCalls(instance);
#endif
            break;
        case DLL_PROCESS_DETACH:
            DebugFoot();
            break;
    }
    return TRUE;
}
