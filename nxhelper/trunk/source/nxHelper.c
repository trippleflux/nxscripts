/*
 * Infernus Library - Tcl extension for the Infernus sitebot.
 * Copyright (c) 2005 Infernus Development Team
 *
 * File Name:
 *   nxHelper.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Tcl extension initialization procedures.
 */

#include <nxHelper.h>

static BOOL initialized = FALSE;
static HMODULE kernelModule;
TCL_DECLARE_MUTEX(initMutex)

/* Global variables. */
OSVERSIONINFO osVersion;
Fn_GetDiskFreeSpaceEx GetDiskFreeSpaceExPtr = NULL;


EXTERN int Nxhelper_Init(Tcl_Interp *interp)
{
    /* Wide integer support was added in Tcl 8.4. */
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
#else /* USE_TCL_STUBS */
    if (Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
#endif /* USE_TCL_STUBS */

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_MutexLock(&initMutex);
    if (!initialized) {
        /* Initialize the OS version structure. */
        osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osVersion);

        kernelModule = LoadLibrary(TEXT("kernel32.dll"));

        if (kernelModule == NULL) {
            Tcl_AppendResult(interp, "unable to load kernel32.dll: ",
                TclSetWinError(interp, GetLastError()), NULL);

            Tcl_MutexUnlock(&initMutex);
            return TCL_ERROR;
        }

        /*
         * These functions must be resolved on run-time for backwards
         * compatibility on older Windows systems (earlier than NT v5).
         */
#ifdef UNICODE
        GetDiskFreeSpaceExPtr = (Fn_GetDiskFreeSpaceEx)
            GetProcAddress(kernelModule, "GetDiskFreeSpaceExW");
#else /* UNICODE */
        GetDiskFreeSpaceExPtr = (Fn_GetDiskFreeSpaceEx)
            GetProcAddress(kernelModule, "GetDiskFreeSpaceExA");
#endif /* UNICODE */

        /*
         * If GetVolumeInformation() is called on a floppy drive or a CD-ROM drive
         * that does not have a disk inserted, the system will display a message box
         * asking the user to insert one.
         */
        SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

        initialized = TRUE;
    }
    Tcl_MutexUnlock(&initMutex);

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }

//  Tcl_CreateObjCommand(interp, "::nx::decode", DecodeObjCmd, NULL, NULL);
//  Tcl_CreateObjCommand(interp, "::nx::encode", EncodeObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::mp3",    Mp3ObjCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::time",   TimeObjCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::touch",  TouchObjCmd,  NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::volume", VolumeObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::zlib",   ZlibObjCmd,   NULL, NULL);

    return TCL_OK;
}


EXTERN int Nxhelper_SafeInit(Tcl_Interp *interp)
{
    return Nxhelper_Init(interp);
}
