/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
 *
 * File Name:
 *   nxHelper.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Tcl extension initialisation procedures.
 */

#include <nxHelper.h>

static BOOL initialised = FALSE;
static HMODULE kernelModule;
TCL_DECLARE_MUTEX(initMutex)

/* Global variables. */
OSVERSIONINFO osVersion;
Fn_GetDiskFreeSpaceEx GetDiskFreeSpaceExPtr = NULL;

EXTERN int Nxhelper_Init(Tcl_Interp *interp);
EXTERN int Nxhelper_SafeInit(Tcl_Interp *interp);
static void Nxhelper_Exit(ClientData dummy);


/*
 * Nxhelper_Init
 *
 *   Initialises the extension for a regular interpreter.
 *
 * Arguments:
 *   interp - Current interpreter.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
int
Nxhelper_Init(Tcl_Interp *interp)
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

    /*
     * Check if the library is already initialised before locking
     * the global initialisation mutex (improves loading time).
     */
    if (!initialised) {
        Tcl_MutexLock(&initMutex);

        /* Check initialisation status again now that we're in the mutex. */
        if (!initialised) {
            /* Initialise the OS version structure. */
            osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&osVersion);

            kernelModule = LoadLibrary(TEXT("kernel32.dll"));
            if (kernelModule != NULL) {
                /*
                 * GetDiskFreeSpaceEx() must be resolved on run-time for backwards
                 * compatibility on older Windows systems (earlier than NT v5).
                 */
#ifdef UNICODE
                GetDiskFreeSpaceExPtr = (Fn_GetDiskFreeSpaceEx)
                    GetProcAddress(kernelModule, "GetDiskFreeSpaceExW");
#else /* UNICODE */
                GetDiskFreeSpaceExPtr = (Fn_GetDiskFreeSpaceEx)
                    GetProcAddress(kernelModule, "GetDiskFreeSpaceExA");
#endif /* UNICODE */
            } else {
                GetDiskFreeSpaceExPtr = NULL;
            }

            /*
             * If GetVolumeInformation() is called on a floppy drive or a CD-ROM drive
             * that does not have a disk inserted, the system will display a message box
             * asking the user to insert one.
             */
            SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

            /* An exit handler should be registered once. */
            Tcl_CreateExitHandler(Nxhelper_Exit, NULL);

            initialised = TRUE;
        }
        Tcl_MutexUnlock(&initMutex);
    }

    Tcl_CreateObjCommand(interp, "::nx::base64", Base64ObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::mp3",    Mp3ObjCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::sleep",  SleepObjCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::time",   TimeObjCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::touch",  TouchObjCmd,  NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::volume", VolumeObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::zlib",   ZlibObjCmd,   NULL, NULL);

    return TCL_OK;
}

/*
 * Nxhelper_SafeInit
 *
 *   Initialises the extension for a safe interpreter.
 *
 * Arguments:
 *   interp - Current interpreter.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
int
Nxhelper_SafeInit(Tcl_Interp *interp)
{
    return Nxhelper_Init(interp);
}

/*
 * Nxhelper_Exit
 *
 *   Cleans up library on exit, frees all state structures.
 *
 * Arguments:
 *   dummy - Not used.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */
static void
Nxhelper_Exit(ClientData dummy)
{
    Tcl_MutexLock(&initMutex);

    if (kernelModule != NULL) {
        FreeLibrary(kernelModule);
        kernelModule = NULL;
    }

    GetDiskFreeSpaceExPtr = NULL;
    initialised = FALSE;

    Tcl_MutexUnlock(&initMutex);

#ifdef TCL_MEM_DEBUG
    Tcl_DumpActiveMemory("MemDump.txt");
#endif
}
