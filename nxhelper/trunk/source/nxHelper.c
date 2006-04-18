/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
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

/* Initialise global variables */
Fn_GetDiskFreeSpaceEx getDiskFreeSpaceExPtr = NULL;
Tcl_HashTable *keyTable = NULL;

/* Local variables */
static BOOL initialised = FALSE;
static HMODULE kernelModule;
static Tcl_Mutex initMutex;

EXTERN Tcl_PackageInitProc Nxhelper_Init;
EXTERN Tcl_PackageInitProc Nxhelper_SafeInit;

static void
ExitHandler(
    ClientData dummy
    );


#ifdef _WINDOWS
/*
 * DllMain
 *
 *   DLL entry point; disables thread library calls.
 *
 * Arguments:
 *   instance - Handle to the DLL module.
 *   reason   - Reason the entry point is being called.
 *   reserved - Not used.
 *
 * Return Value:
 *   Always returns non-zero (success).
 */
BOOL WINAPI
DllMain(
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
#endif /* _WINDOWS */


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
 */
int
Nxhelper_Init(
    Tcl_Interp *interp
    )
{
    /* Wide integer support was added in Tcl 8.4. */
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }

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
                getDiskFreeSpaceExPtr = (Fn_GetDiskFreeSpaceEx)
                    GetProcAddress(kernelModule, "GetDiskFreeSpaceExW");
#else /* UNICODE */
                getDiskFreeSpaceExPtr = (Fn_GetDiskFreeSpaceEx)
                    GetProcAddress(kernelModule, "GetDiskFreeSpaceExA");
#endif /* UNICODE */
            }

            /*
             * If GetVolumeInformation() is called on a floppy drive or a CD-ROM drive
             * that does not have a disk inserted, the system will display a message box
             * asking the user to insert one.
             */
            SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

            /* An exit handler should be registered once. */
            Tcl_CreateExitHandler(ExitHandler, NULL);

            initialised = TRUE;
        }
        Tcl_MutexUnlock(&initMutex);
    }

    /* Create the hash table used for the "::nx::key" command. */
    if (keyTable == NULL) {
        Tcl_MutexLock(&keyMutex);

        /* Check again now that we're in the mutex. */
        if (keyTable == NULL) {
            keyTable = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
            Tcl_InitHashTable(keyTable, TCL_STRING_KEYS);
        }

        Tcl_MutexUnlock(&keyMutex);
    }

    Tcl_CreateObjCommand(interp, "::nx::base64", Base64ObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::key",    KeyObjCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::mp3",    Mp3ObjCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::sleep",  SleepObjCmd,  NULL, NULL);
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
 */
int
Nxhelper_SafeInit(
    Tcl_Interp *interp
    )
{
    return Nxhelper_Init(interp);
}

/*
 * ExitHandler
 *
 *   Cleans up library on exit, frees all state structures.
 *
 * Arguments:
 *   dummy - Not used.
 *
 * Returns:
 *   None.
 */
static void
ExitHandler(
    ClientData dummy
    )
{
    /* Init clean-up. */
    Tcl_MutexLock(&initMutex);
    if (kernelModule != NULL) {
        FreeLibrary(kernelModule);
        kernelModule = NULL;
    }

    getDiskFreeSpaceExPtr = NULL;
    initialised = FALSE;
    Tcl_MutexUnlock(&initMutex);
    Tcl_MutexFinalize(&initMutex);

    /* Key clean-up. */
    Tcl_MutexLock(&keyMutex);
    if (keyTable != NULL) {
        KeyClearTable();
        Tcl_DeleteHashTable(keyTable);

        ckfree((char *)keyTable);
        keyTable = NULL;
    }
    Tcl_MutexUnlock(&keyMutex);
    Tcl_MutexFinalize(&keyMutex);

#ifdef TCL_MEM_DEBUG
    Tcl_DumpActiveMemory("MemDump.txt");
#endif
}
