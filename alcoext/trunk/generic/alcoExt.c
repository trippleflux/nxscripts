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
static ExtState *stateHead = NULL;

TCL_DECLARE_MUTEX(initMutex)
TCL_DECLARE_MUTEX(stateListMutex)

#ifdef _WINDOWS
OSVERSIONINFOA osVersion;
WinProcs winProcs;
#endif // _WINDOWS

static void
FreeState(
    ExtState *state,
    int removeProc
    );

static Tcl_ExitProc ExitHandler;
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
    ExtState *state;

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

    // Allocate state structure.
    state = (ExtState *)ckalloc(sizeof(ExtState));
    state->interp = interp;
    state->next   = NULL;
    state->prev   = NULL;

    state->cryptTable = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(state->cryptTable, TCL_STRING_KEYS);

#ifndef _WINDOWS
    state->glftpdTable = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(state->glftpdTable, TCL_STRING_KEYS);
#endif // !_WINDOWS

    Tcl_MutexLock(&stateListMutex);
    // Insert at the list head.
    if (stateHead == NULL) {
        stateHead = state;
    } else {
        state->next = stateHead;
        stateHead->prev = state;
        stateHead = state;
    }
    Tcl_MutexUnlock(&stateListMutex);

    // Clean up state on interpreter deletion.
    Tcl_CallWhenDeleted(interp, InterpDeleteHandler, (ClientData)state);

    // Create Tcl commands.
    state->cmds[0] = Tcl_CreateObjCommand(interp, "compress", CompressObjCmd, NULL, NULL);
    state->cmds[1] = Tcl_CreateObjCommand(interp, "crypt",    CryptObjCmd,    (ClientData)state, NULL);
    state->cmds[2] = Tcl_CreateObjCommand(interp, "decode",   EncodingObjCmd, (ClientData)decodeFuncts, NULL);
    state->cmds[3] = Tcl_CreateObjCommand(interp, "encode",   EncodingObjCmd, (ClientData)encodeFuncts, NULL);

    //
    // These commands are not created for safe interpreters because
    // they interact with the file system and/or other processes.
    //
    if (!Tcl_IsSafe(interp)) {
        state->cmds[4] = Tcl_CreateObjCommand(interp, "volume", VolumeObjCmd, NULL, NULL);

#ifdef _WINDOWS
        state->cmds[5] = Tcl_CreateObjCommand(interp, "ioftpd", IoFtpdObjCmd, NULL, NULL);
#else // _WINDOWS
        state->cmds[5] = Tcl_CreateObjCommand(interp, "glftpd", GlFtpdObjCmd, (ClientData)state, NULL);
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

#if 0
    int i;
    for (i = 0; i < ARRAYSIZE(state->cmds); i++) {
        Tcl_DeleteCommandFromToken(state->interp, state->cmds[i]);
    }
#endif

    if (flags == TCL_UNLOAD_DETACH_FROM_INTERPRETER) {
        ExtState *stateList;

        Tcl_MutexLock(&stateListMutex);
        for (stateList = stateHead; stateList != NULL; stateList = stateList->next) {

            if (interp == stateList->interp) {
                // Remove the interpreter's state from the list.
                if (stateList->next != NULL) {
                    stateList->next->prev = stateList->prev;
                }
                if (stateList->prev != NULL) {
                    stateList->prev->next = stateList->next;
                }
                if (stateHead == stateList) {
                    stateHead = stateList->next;
                }

                FreeState(stateList, 1);
                break;
            }
        }
        Tcl_MutexUnlock(&stateListMutex);
        return TCL_OK;
    }

    if (flags == TCL_UNLOAD_DETACH_FROM_PROCESS) {
        //
        // During Tcl's finailisation process (after the extension has been
        // unloaded), it will invoke all registered exit handlers.
        //
        Tcl_DeleteExitHandler(ExitHandler, NULL);

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
    state      - Pointer to a "ExtState" structure.

    removeProc - Remove the interp deletion callback.

Return Value:
    None.

--*/
static void
FreeState(
    ExtState *state,
    int removeProc
    )
{

    assert(state != NULL);
    DebugPrint("FreeState: state=%p removeProc=%d\n", state, removeProc);

    if (removeProc) {
        //
        // Once the interpreter is deleted (after the extension has been
        // unloaded), Tcl will invoke all registered interp deletion handlers.
        //
        Tcl_DontCallWhenDeleted(state->interp,
            InterpDeleteHandler, (ClientData)state);
    }

    // Free hash tables.
    CryptCloseHandles(state->cryptTable);
    Tcl_DeleteHashTable(state->cryptTable);
    ckfree((char *)state->cryptTable);

#ifndef _WINDOWS
    GlCloseHandles(state->glftpdTable);
    Tcl_DeleteHashTable(state->glftpdTable);
    ckfree((char *)state->glftpdTable);
#endif // !_WINDOWS

    ckfree((char *)state);
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
    initialised = 0;

#ifdef _WINDOWS
    Tcl_MutexLock(&initMutex);
    if (winProcs.module != NULL) {
        FreeLibrary(winProcs.module);
    }
    ZeroMemory(&winProcs, sizeof(WinProcs));
    Tcl_MutexUnlock(&initMutex);
#endif // _WINDOWS

    Tcl_MutexLock(&stateListMutex);
    if (stateHead != NULL) {
        ExtState *stateList;
        ExtState *nextState;

        // Free all states structures.
        for (stateList = stateHead; stateList != NULL; stateList = nextState) {
            nextState = stateList->next;
            FreeState(stateList, 1);
        }
        stateHead = NULL;
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
    ExtState *state = (ExtState *)clientData;
    ExtState *stateList;

    DebugPrint("InterpDelete: interp=%p state=%p\n", interp, state);
    if (state == NULL) {
        return;
    }

    Tcl_MutexLock(&stateListMutex);
    for (stateList = stateHead; stateList != NULL; stateList = stateList->next) {

        if (state == stateList) {
            // Remove the interpreter's state from the list.
            if (stateList->prev == NULL) {
                stateHead = stateList->next;
                if (stateList->next != NULL) {
                    stateHead->prev = NULL;
                }
            } else if (stateList->next == NULL) {
                stateList->prev->next = NULL;
            } else {
                stateList->prev->next = stateList->next;
                stateList->next->prev = stateList->prev;
            }

            //
            // Tcl 8.5 calls the unload function before the interp deletion
            // handler. Since all states are freed in the unload function,
            // we must only free states present in the global state list.
            //
            FreeState(stateList, 0);
            break;
        }
    }
    Tcl_MutexUnlock(&stateListMutex);
}
