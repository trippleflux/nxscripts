/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Group Management

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Group management functions.

*/

#include "lib.h"

/*++

Io_GroupCreate

    Creates a new group.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    groupName   - Pointer to a null-terminated string that specifies the group
                  name to create.

    groupId     - Receives the group ID of the created group.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GroupCreate(
    IO_MEMORY *memory,
    const char *groupName,
    int *groupId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    // Validate arguments.
    if (memory == NULL || groupName == NULL || groupId == NULL) {
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
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    result = Io_ShmQuery(memory, DC_CREATE_GROUP, 5000);
    if (result != (DWORD)-1) {
        *groupId = (int)result;
        return TRUE;
    }

    *groupId = -1;
    SetLastError(ERROR_INVALID_GROUPNAME);
    return FALSE;
}

/*++

Io_GroupRename

    Renames an existing group.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
                  buffer size must be large enough to hold the DC_RENAME structure.

    groupName   - Pointer to a null-terminated string that specifies an
                  existing group name.

    newName     - Pointer to a null-terminated string that specifies the new
                  name for the group.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GroupRename(
    IO_MEMORY *memory,
    const char *groupName,
    const char *newName
    )
{
    DC_RENAME *dcRename;

    // Validate arguments.
    if (memory == NULL || groupName == NULL || newName == NULL) {
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
    StringCchCopyA(dcRename->tszName,    ARRAYSIZE(dcRename->tszName),    groupName);
    StringCchCopyA(dcRename->tszNewName, ARRAYSIZE(dcRename->tszNewName), newName);

    if (!Io_ShmQuery(memory, DC_RENAME_GROUP, 5000)) {
        return TRUE;
    }

    SetLastError(ERROR_INVALID_GROUPNAME);
    return FALSE;
}

/*++

Io_GroupDelete

    Deletes a group.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    groupName   - Pointer to a null-terminated string that specifies the group
                  to be deleted

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GroupDelete(
    IO_MEMORY *memory,
    const char *groupName
    )
{
    DC_NAMEID *dcNameId;

    // Validate arguments.
    if (memory == NULL || groupName == NULL) {
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
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    if (!Io_ShmQuery(memory, DC_DELETE_GROUP, 5000)) {
        return TRUE;
    }

    SetLastError(ERROR_INVALID_GROUPNAME);
    return FALSE;
}

/*++

Io_GroupGetFile

    Retrieves the GROUPFILE structure for a specified group ID.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the GROUPFILE
                  structure.

    groupId     - Specifies the group ID to look-up.

    groupFile   - Pointer to a GROUPFILE structure that receives the group
                  information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GroupGetFile(
    IO_MEMORY *memory,
    int groupId,
    GROUPFILE *groupFile
    )
{
    // Validate arguments.
    if (memory == NULL || groupFile == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(GROUPFILE)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Set the specified group ID.
    ((GROUPFILE *)memory->block)->Gid = groupId;

    if (!Io_ShmQuery(memory, DC_GROUPFILE_OPEN, 5000)) {
        CopyMemory(groupFile, memory->block, sizeof(GROUPFILE));

        // Close the group-file before returning.
        Io_ShmQuery(memory, DC_GROUPFILE_CLOSE, 5000);
        return TRUE;
    }

    // Clear the group-file on failure.
    ZeroMemory(groupFile, sizeof(GROUPFILE));
    groupFile->Gid = -1;

    SetLastError(ERROR_NO_SUCH_GROUP);
    return FALSE;
}

/*++

Io_GroupSetFile

    Updates the GROUPFILE structure for a specified group ID.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the GROUPFILE
                  structure.

    groupFile   - Pointer to a GROUPFILE structure that contains the new group
                  information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GroupSetFile(
    IO_MEMORY *memory,
    const GROUPFILE *groupFile
    )
{
    DWORD error = ERROR_SUCCESS;

    // Validate arguments.
    if (memory == NULL || groupFile == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(GROUPFILE)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Set the specified group ID.
    ((GROUPFILE *)memory->block)->Gid = groupFile->Gid;

    if (!Io_ShmQuery(memory, DC_GROUPFILE_OPEN, 5000)) {
        if (!Io_ShmQuery(memory, DC_GROUPFILE_LOCK, 5000)) {
            //
            // Copy the GROUPFILE structure to the shared memory block
            // after locking, since the open call will overwrite it.
            //
            // The lpInternal and lpParent members must not be changed!
            //
            CopyMemory(memory->block, groupFile, offsetof(GROUPFILE, lpInternal));

            // Unlock will update the group-file.
            Io_ShmQuery(memory, DC_GROUPFILE_UNLOCK, 5000);
        } else {
            error = ERROR_LOCK_FAILED;
        }

        // Close the group-file before returning.
        Io_ShmQuery(memory, DC_GROUPFILE_CLOSE, 5000);
    } else {
        error = ERROR_NO_SUCH_GROUP;
    }

    if (error == ERROR_SUCCESS) {
        return TRUE;
    }
    SetLastError(error);
    return FALSE;
}

/*++

Io_GroupIdToName

    Resolves a group ID to its corresponding group name.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    groupId     - Specifies the group ID to resolve.

    groupName   - Pointer to the buffer that receives the group's name. The
                  buffer must be able to hold "_MAX_NAME+1" characters.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GroupIdToName(
    IO_MEMORY *memory,
    int groupId,
    char *groupName
    )
{
    DC_NAMEID *dcNameId;

    // Validate arguments.
    if (memory == NULL || groupName == NULL) {
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
    dcNameId->Id = groupId;

    if (!Io_ShmQuery(memory, DC_GID_TO_GROUP, 5000)) {
        StringCchCopyA(groupName, _MAX_NAME+1, dcNameId->tszName);
        return TRUE;
    }

    groupName[0] = '\0';
    SetLastError(ERROR_NO_SUCH_GROUP);
    return FALSE;
}

/*++

Io_GroupNameToId

    Resolves a group name to its corresponding group ID.

Arguments:
    memory      - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.
                  The buffer size must be large enough to hold the DC_NAMEID
                  structure.

    groupName   - Pointer to a null-terminated string that specifies the group
                  name to resolve.

    groupId     - Receives the group's ID.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GroupNameToId(
    IO_MEMORY *memory,
    const char *groupName,
    int *groupId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    // Validate arguments.
    if (memory == NULL || groupName == NULL || groupId == NULL) {
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
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    result = Io_ShmQuery(memory, DC_GROUP_TO_GID, 5000);
    if (result != (DWORD)-1) {
        *groupId = (int)result;
        return TRUE;
    }

    *groupId = -1;
    SetLastError(ERROR_NO_SUCH_GROUP);
    return FALSE;
}
