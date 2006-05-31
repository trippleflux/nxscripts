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

// Globals
#ifdef _WINDOWS
OSVERSIONINFOA osVersion;
WinProcs winProcs;
#endif

// Locals
static unsigned char initialised = 0;
static ExtState *stateHead = NULL;

TCL_DECLARE_MUTEX(initMutex)
TCL_DECLARE_MUTEX(stateListMutex)

static void
Initialise(
    void
    );

static void
Finalise(
    int removeCmds
    );

static void
FreeState(
    ExtState *state,
    int removeCmds,
    int removeProc
    );

static Tcl_ExitProc ExitHandler;
static Tcl_CmdDeleteProc CmdDeleted;
static Tcl_InterpDeleteProc InterpDeleted;


/*++

DllMain

    DLL entry point; disables thread library calls.

Arguments:
    instance - Handle to the DLL module.

    reason   - Reason the entry point is being called.

    reserved - Not used.

Return Value:
    Always returns non-zero (success).

--*/
#ifdef _WINDOWS
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
#endif // _WINDOWS

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
    int i;
    ExtState *state;
    Tcl_CmdInfo cmdInfo;

    DebugPrint("Init: interp=%p\n", interp);

    // Wide integer support was added in Tcl 8.4.
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }

    Initialise();

    // Allocate state structure.
    state = (ExtState *)ckalloc(sizeof(ExtState));
    memset(state, 0, sizeof(ExtState));
    state->interp = interp;

    state->cryptTable = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(state->cryptTable, TCL_STRING_KEYS);

#ifndef _WINDOWS
    state->glftpdTable = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(state->glftpdTable, TCL_STRING_KEYS);
#endif

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
    Tcl_CallWhenDeleted(interp, InterpDeleted, (ClientData)state);

    // Create Tcl commands.
    state->cmds[0] = Tcl_CreateObjCommand(interp, "compress", CompressObjCmd, NULL, CmdDeleted);
    state->cmds[1] = Tcl_CreateObjCommand(interp, "crypt",    CryptObjCmd,    (ClientData)state, CmdDeleted);
    state->cmds[2] = Tcl_CreateObjCommand(interp, "decode",   EncodingObjCmd, (ClientData)decodeFuncts, CmdDeleted);
    state->cmds[3] = Tcl_CreateObjCommand(interp, "encode",   EncodingObjCmd, (ClientData)encodeFuncts, CmdDeleted);

    //
    // These commands are not created for safe interpreters because
    // they interact with the file system and/or other processes.
    //
    if (!Tcl_IsSafe(interp)) {
        state->cmds[4] = Tcl_CreateObjCommand(interp, "volume", VolumeObjCmd, NULL, CmdDeleted);

#ifdef _WINDOWS
        state->cmds[5] = Tcl_CreateObjCommand(interp, "ioftpd", IoFtpdObjCmd, NULL, CmdDeleted);
#else
        state->cmds[5] = Tcl_CreateObjCommand(interp, "glftpd", GlFtpdObjCmd, (ClientData)state, CmdDeleted);
#endif
    }

    // Pass the address of the command token to the deletion handler.
    for (i = 0; i < ARRAYSIZE(state->cmds); i++) {
        if (Tcl_GetCommandInfoFromToken(state->cmds[i], &cmdInfo)) {
            cmdInfo.deleteData = (ClientData)&state->cmds[i];
            Tcl_SetCommandInfoFromToken(state->cmds[i], &cmdInfo);
        }
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

    Unloads the extension from a process or interpreter.

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
        ExtState *state;

        Tcl_MutexLock(&stateListMutex);
        for (state = stateHead; state != NULL; state = state->next) {

            if (interp == state->interp) {
                // Remove the interpreter's state from the list.
                if (state->next != NULL) {
                    state->next->prev = state->prev;
                }
                if (state->prev != NULL) {
                    state->prev->next = state->next;
                }
                if (stateHead == state) {
                    stateHead = state->next;
                }

                FreeState(state, 1, 1);
                break;
            }
        }
        Tcl_MutexUnlock(&stateListMutex);

    } else if (flags == TCL_UNLOAD_DETACH_FROM_PROCESS) {
        // Remove registered exit handlers.
        Tcl_DeleteExitHandler(ExitHandler, NULL);
        Finalise(1);

    } else {
        // Unknown flags value.
        return TCL_ERROR;
    }

    // Unregister the package (there is no Tcl_PkgForget(), or similar).
    Tcl_Eval(interp, "package forget " PACKAGE_NAME);
    return TCL_OK;
}

/*++

Alcoext_SafeUnload

    Unloads the extension from a process or safe interpreter.

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

Initialise

    Initialises the library; allocating and registering resources.

Arguments:
    None.

Return Value:
    None.

--*/
static void
Initialise(
    void
    )
{
    DebugPrint("Initialise: initialised=%d\n", initialised);

    //
    // Check if the library is already initialised before locking
    // the global initialisation mutex (improves loading time).
    //
    if (initialised) {
        return;
    }

    Tcl_MutexLock(&initMutex);

    // Check initialisation status again now that we are in the mutex.
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

        Tcl_CreateExitHandler(ExitHandler, NULL);

        initialised = 1;
    }
    Tcl_MutexUnlock(&initMutex);
}

/*++

Finalise

    Finalises the library; freeing all held resources.

Arguments:
    removeCmds - Remove extension commands from all interpreters.

Return Value:
    None.

--*/
static void
Finalise(
    int removeCmds
    )
{
    DebugPrint("Finalise: removeCmds=%d\n", removeCmds);

#ifdef _WINDOWS
    Tcl_MutexLock(&initMutex);
    if (winProcs.module != NULL) {
        FreeLibrary(winProcs.module);
    }
    ZeroMemory(&winProcs, sizeof(WinProcs));
    Tcl_MutexUnlock(&initMutex);
#endif

    Tcl_MutexFinalize(&initMutex);
    initialised = 0;

    Tcl_MutexLock(&stateListMutex);
    if (stateHead != NULL) {
        ExtState *state;
        ExtState *stateNext;

        // Free all states structures.
        for (state = stateHead; state != NULL; state = stateNext) {
            stateNext = state->next;
            FreeState(state, removeCmds, 1);
        }
        stateHead = NULL;
    }
    Tcl_MutexUnlock(&stateListMutex);
    Tcl_MutexFinalize(&stateListMutex);

#ifdef TCL_MEM_DEBUG
    Tcl_DumpActiveMemory("MemDump.txt");
#endif
}

/*++

FreeState

    Deletes hash tables and frees state structure.

Arguments:
    state      - Pointer to a "ExtState" structure.

    removeCmds - Remove registered commands.

    removeProc - Remove the interp deletion callback.

Return Value:
    None.

--*/
static void
FreeState(
    ExtState *state,
    int removeCmds,
    int removeProc
    )
{
    assert(state != NULL);
    DebugPrint("FreeState: state=%p state->interp=%p removeCmds=%d removeProc=%d\n",
        state, state->interp, removeCmds, removeProc);

    if (removeCmds) {
        int i;
        for (i = 0; i < ARRAYSIZE(state->cmds); i++) {
            if (state->cmds[i] == NULL) {
                continue;
            }
            Tcl_DeleteCommandFromToken(state->interp, state->cmds[i]);
        }
    }

    if (removeProc) {
        Tcl_DontCallWhenDeleted(state->interp, InterpDeleted, (ClientData)state);
    }

    // Free hash tables.
    CryptCloseHandles(state->cryptTable);
    Tcl_DeleteHashTable(state->cryptTable);
    ckfree((char *)state->cryptTable);

#ifndef _WINDOWS
    GlCloseHandles(state->glftpdTable);
    Tcl_DeleteHashTable(state->glftpdTable);
    ckfree((char *)state->glftpdTable);
#endif

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
    Finalise(0);
}

/*++

CmdDeleted

    Sets the address of a command token to null, otherwise the command
    will be deleted again during unload (causing Tcl to crash).

Arguments:
    clientData - Address of a command token.

Return Value:
    None.

--*/
static void
CmdDeleted(
    ClientData clientData
    )
{
    DebugPrint("CmdDeleted: clientData=%p\n", clientData);
    *((Tcl_Command *)clientData) = NULL;
}

/*++

InterpDeleted

    Frees the state structure for an interpreter that is being deleted.

Arguments:
    clientData - Pointer to a "ExtState" structure.

    interp     - Current interpreter.

Return Value:
    None.

--*/
static void
InterpDeleted(
    ClientData clientData,
    Tcl_Interp *interp
    )
{
    ExtState *state;
    ExtState *stateCurrent = (ExtState *)clientData;

    DebugPrint("InterpDeleted: interp=%p state=%p\n", interp, stateCurrent);

    Tcl_MutexLock(&stateListMutex);
    for (state = stateHead; state != NULL; state = state->next) {

        if (state == stateCurrent) {
            // Remove the interpreter's state from the list.
            if (state->prev == NULL) {
                stateHead = state->next;
                if (state->next != NULL) {
                    stateHead->prev = NULL;
                }
            } else if (state->next == NULL) {
                state->prev->next = NULL;
            } else {
                state->prev->next = state->next;
                state->next->prev = state->prev;
            }

            //
            // Tcl 8.5 calls the unload function before the interp deletion
            // handler. Since all states are freed in the unload function,
            // we must only free states present in the global state list.
            //
            FreeState(state, 0, 0);
            break;
        }
    }
    Tcl_MutexUnlock(&stateListMutex);
}
