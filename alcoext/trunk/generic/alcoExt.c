/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoExt.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 16, 2005
 *
 * Abstract:
 *   Tcl extension initialisation procedures.
 */

#include <alcoExt.h>

static unsigned char initialised = 0;
static StateList *stateListHead = NULL;

TCL_DECLARE_MUTEX(initMutex)
/*
 * Access to the state list is guarded with a mutex. This is not the most
 * efficient approach, but higher level synchronization methods are more
 * difficult due to the lack of consistency between platforms.
 */
TCL_DECLARE_MUTEX(stateMutex)

#ifdef __WIN32__
WinProcs winProcs;
OSVERSIONINFOA osVersion;
#endif /* __WIN32__ */

static void FreeState(ExtState *statePtr);
static Tcl_ExitProc         ExitHandler;
static Tcl_InterpDeleteProc InterpDeleteHandler;


/*
 * Alcoext_Init
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
Alcoext_Init(Tcl_Interp *interp)
{
    ExtState *statePtr;
    StateList *stateListPtr;

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

    if (!initialised) {
#ifdef __WIN32__
        /* Initialise the OS version structure. */
        osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        GetVersionExA(&osVersion);

        ZeroMemory(&winProcs, sizeof(WinProcs));
        winProcs.kernelModule = LoadLibraryA("kernel32.dll");

        if (winProcs.kernelModule == NULL) {
            Tcl_AppendResult(interp, "unable to load kernel32.dll: ",
                TclSetWinError(interp, GetLastError()), NULL);

            Tcl_MutexUnlock(&initMutex);
            return TCL_ERROR;
        }

        /*
         * These functions must be resolved on run-time for backwards
         * compatibility on older Windows systems (earlier than NT v5).
         */
        winProcs.getDiskFreeSpaceEx = (Fn_GetDiskFreeSpaceExA)
            GetProcAddress(winProcs.kernelModule, "GetDiskFreeSpaceExA");

        winProcs.findFirstVolumeMountPoint = (Fn_FindFirstVolumeMountPointA)
            GetProcAddress(winProcs.kernelModule, "FindFirstVolumeMountPointA");
        winProcs.findNextVolumeMountPoint = (Fn_FindNextVolumeMountPointA)
            GetProcAddress(winProcs.kernelModule, "FindNextVolumeMountPointA");
        winProcs.findVolumeMountPointClose = (Fn_FindVolumeMountPointClose)
            GetProcAddress(winProcs.kernelModule, "FindVolumeMountPointClose");

        winProcs.getVolumeNameForVolumeMountPoint = (Fn_GetVolumeNameForVolumeMountPointA)
            GetProcAddress(winProcs.kernelModule, "GetVolumeNameForVolumeMountPointA");

        /*
         * If GetVolumeInformation() is called on a floppy drive or a CD-ROM
         * drive that does not have a disk inserted, the system will display a
         * message box asking the user to insert one.
         */
        SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);
#endif /* __WIN32__ */

        /* An exit handler must only be registered once. */
        Tcl_CreateExitHandler(ExitHandler, NULL);

        /* Register ciphers, hashes, and PRNGs for LibTomCrypt. */
        register_cipher(&des3_desc);
        register_cipher(&aes_desc);
        register_cipher(&anubis_desc);
        register_cipher(&blowfish_desc);
        register_cipher(&cast5_desc);
        register_cipher(&des_desc);
        register_cipher(&khazad_desc);
        register_cipher(&noekeon_desc);
        register_cipher(&rc2_desc);
        register_cipher(&rc5_desc);
        register_cipher(&rc6_desc);
        register_cipher(&saferp_desc);
        register_cipher(&safer_k128_desc);
        register_cipher(&safer_k64_desc);
        register_cipher(&safer_sk128_desc);
        register_cipher(&safer_sk64_desc);
        register_cipher(&skipjack_desc);
        register_cipher(&twofish_desc);
        register_cipher(&xtea_desc);
        register_hash(&md2_desc);
        register_hash(&md4_desc);
        register_hash(&md5_desc);
        register_hash(&rmd128_desc);
        register_hash(&rmd160_desc);
        register_hash(&sha1_desc);
        register_hash(&sha224_desc);
        register_hash(&sha256_desc);
        register_hash(&sha384_desc);
        register_hash(&sha512_desc);
        register_hash(&tiger_desc);
        register_hash(&whirlpool_desc);
        register_prng(&fortuna_desc);
        register_prng(&rc4_desc);
        register_prng(&sober128_desc);
        register_prng(&yarrow_desc);

        initialised = 1;
    }
    Tcl_MutexUnlock(&initMutex);

    /* Allocate state structures. */
    stateListPtr = (StateList *) ckalloc(sizeof(StateList));
    statePtr = (ExtState *) ckalloc(sizeof(ExtState));

    statePtr->cryptTable = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(statePtr->cryptTable, TCL_STRING_KEYS);
    statePtr->cryptHandle = 0;

#ifdef __WIN32__
    statePtr->ioTable = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(statePtr->ioTable, TCL_STRING_KEYS);
    statePtr->ioHandle = 0;
#else /* __WIN32__ */
    statePtr->glTable = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(statePtr->glTable, TCL_STRING_KEYS);
    statePtr->glHandle = 0;
#endif /* __WIN32__ */

    /*
     * Since callbacks registered with Tcl_CallWhenDeleted() are not executed in
     * certain situations (calling Tcl_Finalize() or invoking the "exit" command),
     * these resources must be freed by an exit handler registered with
     * Tcl_CreateExitHandler().
     */
    stateListPtr->interp = interp;
    stateListPtr->state  = statePtr;
    stateListPtr->next   = NULL;
    stateListPtr->prev   = NULL;

    Tcl_MutexLock(&stateMutex);
    /* Insert at the list head. */
    if (stateListHead == NULL) {
        stateListHead = stateListPtr;
    } else {
        stateListPtr->next = stateListHead;
        stateListHead->prev = stateListPtr;
        stateListHead = stateListPtr;
    }
    Tcl_MutexUnlock(&stateMutex);

    /* Clean up state on interpreter deletion. */
    Tcl_CallWhenDeleted(interp, InterpDeleteHandler, (ClientData) statePtr);

    /* Create Tcl commands. */
    Tcl_CreateObjCommand(interp, "::alcoholicz::crypt", CryptObjCmd,
        (ClientData) statePtr, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::alcoholicz::decode", EncodingObjCmd,
        (ClientData) decodeFuncts, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::alcoholicz::encode", EncodingObjCmd,
        (ClientData) encodeFuncts, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::alcoholicz::zlib", ZlibObjCmd,
        (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    /*
     * These commands are not created for safe interpreters because
     * they interact with the file system and/or other processes.
     */
    if (!Tcl_IsSafe(interp)) {
        Tcl_CreateObjCommand(interp, "::alcoholicz::volume", VolumeObjCmd,
            (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

#ifdef __WIN32__
        Tcl_CreateObjCommand(interp, "::alcoholicz::ioftpd", IoFtpdObjCmd,
            (ClientData) statePtr, (Tcl_CmdDeleteProc *) NULL);
#else /* __WIN32__ */
        Tcl_CreateObjCommand(interp, "::alcoholicz::glftpd", GlFtpdObjCmd,
            (ClientData) statePtr, (Tcl_CmdDeleteProc *) NULL);
#endif /* __WIN32__ */
    }

    Tcl_Eval(interp, "namespace eval ::alcoholicz {"
        "namespace export crypt encode decode volume zlib "
#ifdef __WIN32__
        "ioftpd "
#else /* __WIN32__ */
        "glftpd "
#endif /* __WIN32__ */
        "}"
        );

    return TCL_OK;
}


/*
 * Alcoext_SafeInit
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
Alcoext_SafeInit(Tcl_Interp *interp)
{
    return Alcoext_Init(interp);
}


/*
 * Alcoext_Unload
 *
 *   Unload the extension from a process or interpreter.
 *
 * Arguments:
 *   interp - Current interpreter.
 *   flags  - Type of detachment.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

int
Alcoext_Unload(Tcl_Interp *interp, int flags)
{
    if (flags == TCL_UNLOAD_DETACH_FROM_INTERPRETER) {
        StateList *stateListPtr;

        Tcl_MutexLock(&stateMutex);
        for (stateListPtr = stateListHead; stateListPtr != NULL; stateListPtr = stateListPtr->next) {

            if (interp == stateListPtr->interp) {
                /* Remove the interpreter's state from the list. */
                if (stateListPtr->prev == NULL) {
                    stateListHead = stateListPtr->next;
                    if (stateListPtr->next != NULL) {
                        stateListHead->prev = NULL;
                    }
                } else if (stateListPtr->next == NULL) {
                    stateListPtr->prev->next = NULL;
                } else {
                    stateListPtr->prev->next = stateListPtr->next;
                    stateListPtr->next->prev = stateListPtr->prev;
                }

                ckfree((char *) stateListPtr->state);
                ckfree((char *) stateListPtr);
                break;
            }
        }
        Tcl_MutexUnlock(&stateMutex);
        return TCL_OK;

    } else if (flags == TCL_UNLOAD_DETACH_FROM_PROCESS) {
        ExitHandler(NULL);
        return TCL_OK;
    }

    /* Unknown 'flags' value. */
    return TCL_ERROR;
}


/*
 * Alcoext_SafeUnload
 *
 *   Unload the extension from a process or safe interpreter.
 *
 * Arguments:
 *   interp - Current interpreter.
 *   flags  - Type of detachment.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

int
Alcoext_SafeUnload(Tcl_Interp *interp, int flags)
{
    return Alcoext_Unload(interp, flags);
}


/*
 * FreeState
 *
 *   Deletes hash tables and frees state structure.
 *
 * Arguments:
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */

static void
FreeState(ExtState *statePtr)
{
    if (statePtr != NULL) {
        CryptCloseHandles(statePtr->cryptTable);
        Tcl_DeleteHashTable(statePtr->cryptTable);
        ckfree((char *) statePtr->cryptTable);

#ifdef __WIN32__
        /* TODO: IoCloseHandles(statePtr->ioTable); */
        Tcl_DeleteHashTable(statePtr->ioTable);
        ckfree((char *) statePtr->ioTable);
#else /* __WIN32__ */

        GlCloseHandles(statePtr->glTable);
        Tcl_DeleteHashTable(statePtr->glTable);
        ckfree((char *) statePtr->glTable);
#endif /* __WIN32__ */

        ckfree((char *) statePtr);
        statePtr = NULL;
    }
}


/*
 * ExitHandler
 *
 *   Cleans up library on exit, frees all state structures
 *   for every interpreter this extension was loaded in.
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
ExitHandler(ClientData dummy)
{
    Tcl_MutexLock(&initMutex);
#ifdef __WIN32__
    if (winProcs.kernelModule != NULL) {
        FreeLibrary(winProcs.kernelModule);
        winProcs.kernelModule = NULL;
    }
#endif /* __WIN32__ */

    initialised = 0;
    Tcl_MutexUnlock(&initMutex);

    Tcl_MutexLock(&stateMutex);
    if (stateListHead != NULL) {
        StateList *stateListPtr;
        StateList *nextStateListPtr;

        /* Free all states structures. */
        for (stateListPtr = stateListHead; stateListPtr != NULL; stateListPtr = nextStateListPtr) {
            nextStateListPtr = stateListPtr->next;

            FreeState(stateListPtr->state);
            ckfree((char *) stateListPtr);
        }
        stateListHead = NULL;
    }
    Tcl_MutexUnlock(&stateMutex);

#ifdef TCL_MEM_DEBUG
    Tcl_DumpActiveMemory("MemDump.txt");
#endif
}


/*
 * InterpDeleteHandler
 *
 *   Frees the state structure for an interpreter that is being deleted.
 *
 * Arguments:
 *   clientData - Pointer to a 'ExtState' structure.
 *   interp     - Current interpreter.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */

static void
InterpDeleteHandler(ClientData clientData, Tcl_Interp *interp)
{
    ExtState *statePtr = (ExtState *) clientData;
    StateList *stateListPtr;

    if (statePtr == NULL) {
        return;
    }

    Tcl_MutexLock(&stateMutex);
    for (stateListPtr = stateListHead; stateListPtr != NULL; stateListPtr = stateListPtr->next) {

        if (statePtr == stateListPtr->state) {
            /* Remove the interpreter's state from the list. */
            if (stateListPtr->prev == NULL) {
                stateListHead = stateListPtr->next;
                if (stateListPtr->next != NULL) {
                    stateListHead->prev = NULL;
                }
            } else if (stateListPtr->next == NULL) {
                stateListPtr->prev->next = NULL;
            } else {
                stateListPtr->prev->next = stateListPtr->next;
                stateListPtr->next->prev = stateListPtr->prev;
            }

            ckfree((char *) stateListPtr);
            break;
        }
    }
    Tcl_MutexUnlock(&stateMutex);

    FreeState(statePtr);
}
