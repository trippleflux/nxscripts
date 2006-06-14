/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    DLL Entry

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Entry point for dynamic libraries.

*/

#include "lib.h"

// Silence C4100: unreferenced formal parameter
#pragma warning(disable : 4100)

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
    // The static CRT requires thread notifications, and we only use the CRT in debug builds.
#if defined(DYNAMIC_CRT) || (defined(STATIC_CRT) && !defined(DEBUG))
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
    }
#endif
    return TRUE;
}
