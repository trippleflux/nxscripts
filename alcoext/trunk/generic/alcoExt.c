/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Extension

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    This module implements the Tcl extension entry point, called when the
    extension is loaded into a interpreter. There is also support for extension
    unloading, implemented in Tcl 8.5.

--*/

#include <alcoExt.h>

static unsigned char initialised = 0;
static StateList *stateListHead = NULL;

TCL_DECLARE_MUTEX(initMutex)
//
// Access to the state list is guarded with a mutex. This is not the most
// efficient approach, but higher level synchronization methods are more
// difficult due to the lack of consistency between platforms.
//
TCL_DECLARE_MUTEX(stateListMutex)

#ifdef _WINDOWS
OSVERSIONINFOA osVersion;
WinProcs winProcs;
#endif // _WINDOWS

static void FreeState(ExtState *statePtr);
static Tcl_ExitProc         ExitHandler;
static Tcl_InterpDeleteProc InterpDeleteHandler;


/*++

Alcoext_Init

    Initialises the extension for a regular interpreter.

Arguments:
    interp - Current interpreter.

Return Value:
    A standard Tcl result.

--*/
int
Alcoext_Init(
    Tcl_Interp *interp
    )
{
    ExtState *statePtr;
    StateList *stateListPtr;

    DebugPrint("Init: interp=%p\n", interp);

    // Wide integer support was added in Tcl 8.4.
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
#else // USE_TCL_STUBS
    if (Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
#endif // USE_TCL_STUBS

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }

    //
    // Check if the library is already initialised before locking
    // the global initialisation mutex (improves loading time).
    //
    if (!initialised) {
        Tcl_MutexLock(&initMutex);

        // Check initialisation status again now that we're in the mutex.
        if (!initialised) {
#ifdef _WINDOWS
            // Initialise the OS version structure.
            osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            GetVersionExA(&osVersion);

            ZeroMemory(&winProcs, sizeof(WinProcs));
            winProcs.module = LoadLibraryA("kernel32.dll");

            if (winProcs.module != NULL) {
                //
                // These functions must be resolved on run-time for backwards
                // compatibility on older Windows systems (earlier than NT v5).
                //
                winProcs.getDiskFreeSpaceEx = (GetDiskFreeSpaceExProc)
                    GetProcAddress(winProcs.module, "GetDiskFreeSpaceExA");

                winProcs.findFirstVolumeMountPoint = (FindFirstVolumeMountPointProc)
                    GetProcAddress(winProcs.module, "FindFirstVolumeMountPointA");
                winProcs.findNextVolumeMountPoint = (FindNextVolumeMountPointProc)
                    GetProcAddress(winProcs.module, "FindNextVolumeMountPointA");
                winProcs.findVolumeMountPointClose = (FindVolumeMountPointCloseProc)
                    GetProcAddress(winProcs.module, "FindVolumeMountPointClose");

                winProcs.getVolumeNameForVolumeMountPoint = (GetVolumeNameForVolumeMountPointProc)
                    GetProcAddress(winProcs.module, "GetVolumeNameForVolumeMountPointA");
            }

            //
            // If GetVolumeInformation() is called on a floppy drive or a CD-ROM
            // drive that does not have a disk inserted, the system will display
            // a message box asking the user to insert one.
            //
            SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);
#endif // _WINDOWS

            // An exit handler must only be registered once.
            Tcl_CreateExitHandler(ExitHandler, NULL);

            // Register ciphers, hashes, and PRNGs for LibTomCrypt.
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
            register_prng(&sprng_desc);
            register_prng(&yarrow_desc);

            initialised = 1;
        }
        Tcl_MutexUnlock(&initMutex);
    }

    // Allocate state structures.
    stateListPtr = (StateList *)ckalloc(sizeof(StateList));
    statePtr = (ExtState *)ckalloc(sizeof(ExtState));

    statePtr->cryptTable = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(statePtr->cryptTable, TCL_STRING_KEYS);

#ifndef _WINDOWS
    statePtr->glftpdTable = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(statePtr->glftpdTable, TCL_STRING_KEYS);
#endif // !_WINDOWS

    //
    // Since callbacks registered with Tcl_CallWhenDeleted() are not executed in
    // certain situations (calling Tcl_Finalize() or invoking the "exit" command),
    // these resources must be freed by an exit handler registered with
    // Tcl_CreateExitHandler().
    //
    stateListPtr->interp = interp;
    stateListPtr->state  = statePtr;
    stateListPtr->next   = NULL;
    stateListPtr->prev   = NULL;

    Tcl_MutexLock(&stateListMutex);
    // Insert at the list head.
    if (stateListHead == NULL) {
        stateListHead = stateListPtr;
    } else {
        stateListPtr->next = stateListHead;
        stateListHead->prev = stateListPtr;
        stateListHead = stateListPtr;
    }
    Tcl_MutexUnlock(&stateListMutex);

    // Clean up state on interpreter deletion.
    Tcl_CallWhenDeleted(interp, InterpDeleteHandler, (ClientData)statePtr);

    // Create Tcl commands.
    Tcl_CreateObjCommand(interp, "compress", CompressObjCmd, (ClientData)NULL, NULL);
    Tcl_CreateObjCommand(interp, "crypt",    CryptObjCmd,    (ClientData)statePtr, NULL);
    Tcl_CreateObjCommand(interp, "decode",   EncodingObjCmd, (ClientData)decodeFuncts, NULL);
    Tcl_CreateObjCommand(interp, "encode",   EncodingObjCmd, (ClientData)encodeFuncts, NULL);

    //
    // These commands are not created for safe interpreters because
    // they interact with the file system and/or other processes.
    //
    if (!Tcl_IsSafe(interp)) {
        Tcl_CreateObjCommand(interp, "volume", VolumeObjCmd, (ClientData)NULL, NULL);

#ifdef _WINDOWS
        Tcl_CreateObjCommand(interp, "ioftpd", IoFtpdObjCmd, (ClientData)NULL, NULL);
#else // _WINDOWS
        Tcl_CreateObjCommand(interp, "glftpd", GlFtpdObjCmd, (ClientData)statePtr, NULL);
#endif // _WINDOWS
    }

    return TCL_OK;
}

/*++

Alcoext_SafeInit

    Initialises the extension for a safe interpreter.

Arguments:
    interp - Current interpreter.

Return Value:
    A standard Tcl result.

--*/
int
Alcoext_SafeInit(
    Tcl_Interp *interp
    )
{
    return Alcoext_Init(interp);
}

/*++

Alcoext_Unload

    Unload the extension from a process or interpreter. As proposed
    in Tcl TIP #100 (http://www.tcl.tk/cgi-bin/tct/tip/100.html).

Arguments:
    interp - Current interpreter.

    flags  - Type of detachment.

Return Value:
    A standard Tcl result.

--*/
int
Alcoext_Unload(
    Tcl_Interp *interp,
    int flags
    )
{
    DebugPrint("Unload: interp=%p flags=%d\n", interp, flags);

    if (flags == TCL_UNLOAD_DETACH_FROM_INTERPRETER) {
        StateList *stateListPtr;

        Tcl_MutexLock(&stateListMutex);
        for (stateListPtr = stateListHead; stateListPtr != NULL; stateListPtr = stateListPtr->next) {

            if (interp == stateListPtr->interp) {
                // Remove the interpreter's state from the list.
                if (stateListPtr->next != NULL) {
                    stateListPtr->next->prev = stateListPtr->prev;
                }
                if (stateListPtr->prev != NULL) {
                    stateListPtr->prev->next = stateListPtr->next;
                }
                if (stateListHead == stateListPtr) {
                    stateListHead = stateListPtr->next;
                }

                FreeState(stateListPtr->state);
                ckfree((char *)stateListPtr);
                break;
            }
        }
        Tcl_MutexUnlock(&stateListMutex);
        return TCL_OK;

    } else if (flags == TCL_UNLOAD_DETACH_FROM_PROCESS) {

        ExitHandler(NULL);
        return TCL_OK;
    }

    // Unknown "flags" value.
    return TCL_ERROR;
}

/*++

Alcoext_SafeUnload

    Unload the extension from a process or safe interpreter.

Arguments:
    interp - Current interpreter.

    flags  - Type of detachment.

Return Value:
    A standard Tcl result.

--*/
int
Alcoext_SafeUnload(
    Tcl_Interp *interp,
    int flags
    )
{
    return Alcoext_Unload(interp, flags);
}

/*++

FreeState

    Deletes hash tables and frees state structure.

Arguments:
    statePtr - Pointer to a "ExtState" structure.

Return Value:
    None.

--*/
static void
FreeState(
    ExtState *statePtr
    )
{
    DebugPrint("FreeState: statePtr=%p\n", statePtr);

    if (statePtr != NULL) {
        CryptCloseHandles(statePtr->cryptTable);
        Tcl_DeleteHashTable(statePtr->cryptTable);
        ckfree((char *)statePtr->cryptTable);

#ifndef _WINDOWS
        GlCloseHandles(statePtr->glftpdTable);
        Tcl_DeleteHashTable(statePtr->glftpdTable);
        ckfree((char *)statePtr->glftpdTable);
#endif // !_WINDOWS

        ckfree((char *)statePtr);
    }
}

/*++

ExitHandler

    Cleans up library on exit, frees all state structures
    for every interpreter this extension was loaded in.

Arguments:
    dummy - Not used.

Return Value:
    None.

--*/
static void
ExitHandler(
    ClientData dummy
    )
{
    DebugPrint("ExitHandler: none\n");

    Tcl_MutexLock(&initMutex);
#ifdef _WINDOWS
    if (winProcs.module != NULL) {
        FreeLibrary(winProcs.module);
    }
    ZeroMemory(&winProcs, sizeof(WinProcs));
#endif // _WINDOWS

    initialised = 0;
    Tcl_MutexUnlock(&initMutex);

    Tcl_MutexLock(&stateListMutex);
    if (stateListHead != NULL) {
        StateList *stateListPtr;
        StateList *nextStateListPtr;

        // Free all states structures.
        for (stateListPtr = stateListHead; stateListPtr != NULL; stateListPtr = nextStateListPtr) {
            nextStateListPtr = stateListPtr->next;

            FreeState(stateListPtr->state);
            ckfree((char *)stateListPtr);
        }
        stateListHead = NULL;
    }
    Tcl_MutexUnlock(&stateListMutex);

#ifdef TCL_MEM_DEBUG
    Tcl_DumpActiveMemory("MemDump.txt");
#endif
}

/*++

InterpDeleteHandler

    Frees the state structure for an interpreter that is being deleted.

Arguments:
    clientData - Pointer to a "ExtState" structure.

    interp     - Current interpreter.

Return Value:
    None.

--*/
static void
InterpDeleteHandler(
    ClientData clientData,
    Tcl_Interp *interp
    )
{
    ExtState *statePtr = (ExtState *)clientData;
    StateList *stateListPtr;

    DebugPrint("InterpDelete: interp=%p statePtr=%p\n", interp, statePtr);
    if (statePtr == NULL) {
        return;
    }

    Tcl_MutexLock(&stateListMutex);
    for (stateListPtr = stateListHead; stateListPtr != NULL; stateListPtr = stateListPtr->next) {

        if (statePtr == stateListPtr->state) {
            // Remove the interpreter's state from the list.
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

            //
            // Tcl 8.5 calls the unload function before the interp deletion
            // handler. Since all states are freed in the unload function,
            // we must only free states present in the global state list.
            //
            FreeState(stateListPtr->state);
            ckfree((char *)stateListPtr);
            break;
        }
    }
    Tcl_MutexUnlock(&stateListMutex);
}
