/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    User Management

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    User management functions.

*/

#include "lib.h"

/*++

Io_UserCreate

    Creates a new user.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    userName    - Pointer to a null-terminated string that specifies the user
                  name to create.

    userId      - Receives the user ID of the created user.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

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

    // Validate arguments.
    if (memory == NULL || userName == NULL || userId == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_NAMEID)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    result = Io_ShmQuery(memory, DC_CREATE_USER, 5000);
    if (result != (DWORD)-1) {
        *userId = (int)result;
        return TRUE;
    }

    *userId = -1;
    SetLastError(ERROR_BAD_USERNAME);
    return FALSE;
}

/*++

Io_UserRename

    Renames an existing user.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
                  buffer size must be large enough to hold the DC_RENAME structure.

    userName    - Pointer to a null-terminated string that specifies an
                  existing user name.

    newName     - Pointer to a null-terminated string that specifies the new
                  name for the user.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

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

    // Validate arguments.
    if (memory == NULL || userName == NULL || newName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_RENAME)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the DC_RENAME structure.
    dcRename = (DC_RENAME *)memory->block;
    StringCchCopyA(dcRename->tszName,    ARRAYSIZE(dcRename->tszName),    userName);
    StringCchCopyA(dcRename->tszNewName, ARRAYSIZE(dcRename->tszNewName), newName);

    if (!Io_ShmQuery(memory, DC_RENAME_USER, 5000)) {
        return TRUE;
    }

    SetLastError(ERROR_BAD_USERNAME);
    return FALSE;
}

/*++

Io_UserDelete

    Deletes a user.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    userName    - Pointer to a null-terminated string that specifies the user
                  to be deleted

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
STDCALL
Io_UserDelete(
    IO_MEMORY *memory,
    const char *userName
    )
{
    DC_NAMEID *dcNameId;

    // Validate arguments.
    if (memory == NULL || userName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_NAMEID)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    if (!Io_ShmQuery(memory, DC_DELETE_USER, 5000)) {
        return TRUE;
    }

    SetLastError(ERROR_BAD_USERNAME);
    return FALSE;
}

/*++

Io_UserGetFile

    Retrieves the USERFILE structure for a specified user ID.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the USERFILE
                  structure.

    userId      - Specifies the user ID to look-up.

    userFile    - Pointer to a USERFILE structure that receives the user
                  information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
STDCALL
Io_UserGetFile(
    IO_MEMORY *memory,
    int userId,
    USERFILE *userFile
    )
{
    // Validate arguments.
    if (memory == NULL || userFile == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(USERFILE)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Set the specified user ID.
    ((USERFILE *)memory->block)->Uid = userId;

    if (!Io_ShmQuery(memory, DC_USERFILE_OPEN, 5000)) {
        CopyMemory(userFile, memory->block, sizeof(USERFILE));

        // Close the user file before returning.
        Io_ShmQuery(memory, DC_USERFILE_CLOSE, 5000);
        return TRUE;
    }

    // Clear the user file on failure.
    ZeroMemory(userFile, sizeof(USERFILE));
    userFile->Uid = -1;

    SetLastError(ERROR_NO_SUCH_USER);
    return FALSE;
}

/*++

Io_UserSetFile

    Updates the USERFILE structure for a specified user ID.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the USERFILE
                  structure.

    userFile    - Pointer to a USERFILE structure that contains the new user
                  information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
STDCALL
Io_UserSetFile(
    IO_MEMORY *memory,
    const USERFILE *userFile
    )
{
    DWORD error = ERROR_SUCCESS;

    // Validate arguments.
    if (memory == NULL || userFile == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(USERFILE)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Set the specified user ID.
    ((USERFILE *)memory->block)->Uid = userFile->Uid;

    if (!Io_ShmQuery(memory, DC_USERFILE_OPEN, 5000)) {
        if (!Io_ShmQuery(memory, DC_USERFILE_LOCK, 5000)) {
            //
            // Copy the USERFILE structure to the shared memory block
            // after locking, since the open call will overwrite it.
            //
            // The lpInternal and lpParent members must not be changed!
            //
            CopyMemory(memory->block, userFile, offsetof(USERFILE, lpInternal));

            // Unlock will update the user file.
            Io_ShmQuery(memory, DC_USERFILE_UNLOCK, 5000);
        } else {
            error = ERROR_LOCK_FAILED;
        }

        // Close the user file before returning.
        Io_ShmQuery(memory, DC_USERFILE_CLOSE, 5000);
    } else {
        error = ERROR_NO_SUCH_USER;
    }

    if (error == ERROR_SUCCESS) {
        return TRUE;
    }
    SetLastError(error);
    return FALSE;
}

/*++

Io_UserIdToName

    Resolves a user ID to its corresponding user name.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    userId      - Specifies the user ID to resolve.

    userName    - Pointer to the buffer that receives the user's name. The
                  buffer must be able to hold "_MAX_NAME+1" characters.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

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

    // Validate arguments.
    if (memory == NULL || userName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_NAMEID)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    dcNameId->Id = userId;

    if (!Io_ShmQuery(memory, DC_UID_TO_USER, 5000)) {
        StringCchCopyA(userName, _MAX_NAME+1, dcNameId->tszName);
        return TRUE;
    }

    userName[0] = '\0';
    SetLastError(ERROR_NO_SUCH_USER);
    return FALSE;
}

/*++

Io_UserNameToId

    Resolves a user name to its corresponding user ID.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    userName    - Pointer to a null-terminated string that specifies the user
                  name to resolve.

    userId      - Receives the user's ID.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

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

    // Validate arguments.
    if (memory == NULL || userName == NULL || userId == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_NAMEID)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    result = Io_ShmQuery(memory, DC_USER_TO_UID, 5000);
    if (result != (DWORD)-1) {
        *userId = (int)result;
        return TRUE;
    }

    *userId = -1;
    SetLastError(ERROR_NO_SUCH_USER);
    return FALSE;
}
