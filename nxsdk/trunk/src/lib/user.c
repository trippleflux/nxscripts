/*++

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    User Management

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    User related functions.

--*/

#include "lib.h"

/*++

Io_UserCreate

    Creates a new user.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    userName    - The user name to create.

    userId      - Location to store the user ID of the created user.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_UserCreate(
    IO_MEMORY *memory,
    const char *userName,
    int *userId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    assert(userId   != NULL);
    DebugPrint("Io_UserCreate: userName=%s userId=0x%p\n", userName, userId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    result = Io_ShmQuery(memory, DC_CREATE_USER, 5000);
    if (result != (DWORD)-1) {
        *userId = (int)result;
        DebugPrint("Io_UserCreate: OKAY\n");
        return TRUE;
    }

    *userId = -1;
    DebugPrint("Io_UserCreate: FAIL\n");
    return FALSE;
}

/*++

Io_UserRename

    Renames an existing user.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_RENAME structure.

    userName    - The user name to rename.

    newName     - The new user name.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_UserRename(
    IO_MEMORY *memory,
    const char *userName,
    const char *newName
    )
{
    DC_RENAME *dcRename;

    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_RENAME));
    assert(userName != NULL);
    assert(newName  != NULL);
    DebugPrint("Io_UserRename: userName=%s newName=%s\n", userName, newName);

    // Initialise the DC_RENAME structure.
    dcRename = (DC_RENAME *)memory->block;
    StringCchCopyA(dcRename->tszName,    ARRAYSIZE(dcRename->tszName),    userName);
    StringCchCopyA(dcRename->tszNewName, ARRAYSIZE(dcRename->tszNewName), newName);

    if (!Io_ShmQuery(memory, DC_RENAME_USER, 5000)) {
        DebugPrint("Io_UserRename: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_UserRename: FAIL\n");
    return FALSE;
}

/*++

Io_UserDelete

    Deletes a user.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    userName    - The user name to delete.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_UserDelete(
    IO_MEMORY *memory,
    const char *userName
    )
{
    DC_NAMEID *dcNameId;

    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    DebugPrint("Io_UserDelete: userName=%s\n", userName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    if (!Io_ShmQuery(memory, DC_DELETE_USER, 5000)) {
        DebugPrint("Io_UserDelete: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_UserDelete: FAIL\n");
    return FALSE;
}

/*++

Io_UserGetFile

    Retrieves the USERFILE structure for a given a user ID.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the USERFILE structure.

    userId      - The user ID to look up.

    userFile    - Pointer to a buffer to receive the USERFILE structure.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_UserGetFile(
    IO_MEMORY *memory,
    int userId,
    USERFILE *userFile
    )
{
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(USERFILE));
    assert(userFile != NULL);
    DebugPrint("Io_UserGetFile: userId=%d userFile=0x%p\n", userId, userFile);

    // Set the requested user ID.
    ((USERFILE *)memory->block)->Uid = userId;

    if (!Io_ShmQuery(memory, DC_USERFILE_OPEN, 5000)) {
        CopyMemory(userFile, memory->block, sizeof(USERFILE));

        // Close the user-file before returning.
        Io_ShmQuery(memory, DC_USERFILE_CLOSE, 5000);

        DebugPrint("Io_UserGetFile: OKAY\n");
        return TRUE;
    }

    // Clear the user-file on failure.
    ZeroMemory(userFile, sizeof(USERFILE));
    userFile->Uid = -1;
    userFile->Gid = -1;

    DebugPrint("Io_UserGetFile: FAIL\n");
    return FALSE;
}

/*++

Io_UserSetFile

    Updates the USERFILE structure for a user.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the USERFILE structure.

    userFile    - Pointer to an initialised USERFILE structure.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_UserSetFile(
    IO_MEMORY *memory,
    const USERFILE *userFile
    )
{
    BOOL status = FALSE;

    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(USERFILE));
    assert(userFile != NULL);
    DebugPrint("Io_UserSetFile: userFile=0x%p userFile->Uid=%d\n", userFile, userFile->Uid);

    // Set the requested user ID.
    ((USERFILE *)memory->block)->Uid = userFile->Uid;

    if (!Io_ShmQuery(memory, DC_USERFILE_OPEN, 5000)) {
        if (!Io_ShmQuery(memory, DC_USERFILE_LOCK, 5000)) {
            //
            // Copy the USERFILE structure to the shared memory block
            // after locking, since the open call will overwrite it.
            // The lpInternal and lpParent members must not be changed!
            //
            CopyMemory(memory->block, userFile, offsetof(USERFILE, lpInternal));

            // Unlock will update the user-file.
            Io_ShmQuery(memory, DC_USERFILE_UNLOCK, 5000);

            status = TRUE;
            DebugPrint("Io_UserSetFile: OKAY\n");
        } else {
            DebugPrint("Io_UserSetFile: LOCK FAIL\n");
        }

        // Close the user-file before returning.
        Io_ShmQuery(memory, DC_USERFILE_CLOSE, 5000);
    } else {
        DebugPrint("Io_UserSetFile: OPEN FAIL\n");
    }

    return status;
}

/*++

Io_UserIdToName

    Resolves a user ID to its corresponding user name.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    userId      - The user ID to resolve.

    userName    - Pointer to a buffer to receive the user name. The
                  buffer must be able to hold _MAX_NAME+1 characters.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_UserIdToName(
    IO_MEMORY *memory,
    int userId,
    char *userName
    )
{
    DC_NAMEID *dcNameId;

    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    DebugPrint("Io_UserIdToName: userId=%d userName=0x%p\n", userId, userName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    dcNameId->Id = userId;

    if (!Io_ShmQuery(memory, DC_UID_TO_USER, 5000)) {
        StringCchCopyA(userName, _MAX_NAME+1, dcNameId->tszName);

        DebugPrint("Io_UserIdToName: OKAY\n");
        return TRUE;
    }

    userName[0] = '\0';
    DebugPrint("Io_UserIdToName: FAIL\n");
    return FALSE;
}

/*++

Io_UserNameToId

    Resolves a user name to its corresponding user ID.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    userName    - The user name to resolve.

    userId      - Location to store the user ID.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_UserNameToId(
    IO_MEMORY *memory,
    const char *userName,
    int *userId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    assert(userId   != NULL);
    DebugPrint("Io_UserNameToId: userName=%s userId=0x%p\n", userName, userId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    result = Io_ShmQuery(memory, DC_USER_TO_UID, 5000);
    if (result != (DWORD)-1) {
        *userId = (int)result;
        DebugPrint("Io_UserNameToId: OKAY\n");
        return TRUE;
    }

    *userId = -1;
    DebugPrint("Io_UserNameToId: FAIL\n");
    return FALSE;
}
