/*++

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    DLL Entry

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Entry point for dynamic libraries.

--*/

#include "lib.h"

/*

DllEntry

    DLL entry point.

Arguments:
    instance - Handle to the DLL module.

    reason   - Reason the entry point is being called.

    reserved - Not used.

Return Value:
    Always returns non-zero (success).

*/
BOOL
WINAPI
DllEntry(
    HINSTANCE instance,
    DWORD reason,
    LPVOID reserved
    )
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
