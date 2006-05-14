/*++

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Group Management

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Group related functions.

--*/

#include "lib.h"

/*++

Io_GroupCreate

    Creates a new group.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    groupName   - The group name to create.

    groupId     - Location to store the group ID of the created group.

Return Value:
    A standard boolean result.

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

    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    assert(groupId   != NULL);
    DebugPrint("Io_GroupCreate: groupName=%s groupId=0x%p\n", groupName, groupId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    result = Io_ShmQuery(memory, DC_CREATE_GROUP, 5000);
    if (result != (DWORD)-1) {
        *groupId = (int)result;
        DebugPrint("Io_GroupCreate: OKAY\n");
        return TRUE;
    }

    *groupId = -1;
    DebugPrint("Io_GroupCreate: FAIL\n");
    return FALSE;
}

/*++

Io_GroupRename

    Renames an existing group.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_RENAME structure.

    groupName   - The group name to rename.

    newName     - The new group name.

Return Value:
    A standard boolean result.

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

    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_RENAME));
    assert(groupName != NULL);
    assert(newName   != NULL);
    DebugPrint("Io_GroupRename: groupName=%s newName=%s\n", groupName, newName);

    // Initialise the DC_RENAME structure.
    dcRename = (DC_RENAME *)memory->block;
    StringCchCopyA(dcRename->tszName,    ARRAYSIZE(dcRename->tszName),    groupName);
    StringCchCopyA(dcRename->tszNewName, ARRAYSIZE(dcRename->tszNewName), newName);

    if (!Io_ShmQuery(memory, DC_RENAME_GROUP, 5000)) {
        DebugPrint("Io_GroupRename: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_GroupRename: FAIL\n");
    return FALSE;
}

/*++

Io_GroupDelete

    Deletes a group.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    groupName   - The group name to delete.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_GroupDelete(
    IO_MEMORY *memory,
    const char *groupName
    )
{
    DC_NAMEID *dcNameId;

    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    DebugPrint("Io_GroupDelete: groupName=%s\n", groupName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    if (!Io_ShmQuery(memory, DC_DELETE_GROUP, 5000)) {
        DebugPrint("Io_GroupDelete: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_GroupDelete: FAIL\n");
    return FALSE;
}

/*++

Io_GroupGetFile

    Retrieves the GROUPFILE structure for a given a group ID.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the GROUPFILE structure.

    groupId     - The group ID to look up.

    groupFile   - Pointer to a buffer to receive the GROUPFILE structure.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_GroupGetFile(
    IO_MEMORY *memory,
    int groupId,
    GROUPFILE *groupFile
    )
{
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(GROUPFILE));
    assert(groupFile != NULL);
    DebugPrint("Io_GroupGetFile: groupId=%d groupFile=0x%p\n", groupId, groupFile);

    // Set the requested group ID.
    ((GROUPFILE *)memory->block)->Gid = groupId;

    if (!Io_ShmQuery(memory, DC_GROUPFILE_OPEN, 5000)) {
        CopyMemory(groupFile, memory->block, sizeof(GROUPFILE));

        // Close the group-file before returning.
        Io_ShmQuery(memory, DC_GROUPFILE_CLOSE, 5000);

        DebugPrint("Io_GroupGetFile: OKAY\n");
        return TRUE;
    }

    // Clear the group-file on failure.
    ZeroMemory(groupFile, sizeof(GROUPFILE));
    groupFile->Gid = -1;

    DebugPrint("Io_GroupGetFile: FAIL\n");
    return FALSE;
}

/*++

Io_GroupSetFile

    Updates the GROUPFILE structure for a group.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the GROUPFILE structure.

    groupFile   - Pointer to an initialised GROUPFILE structure.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_GroupSetFile(
    IO_MEMORY *memory,
    const GROUPFILE *groupFile
    )
{
    BOOL status = FALSE;

    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(GROUPFILE));
    assert(groupFile != NULL);
    DebugPrint("Io_GroupSetFile: groupFile=0x%p groupFile->Gid=%d\n", groupFile, groupFile->Gid);

    // Set the requested group ID.
    ((GROUPFILE *)memory->block)->Gid = groupFile->Gid;

    if (!Io_ShmQuery(memory, DC_GROUPFILE_OPEN, 5000)) {
        if (!Io_ShmQuery(memory, DC_GROUPFILE_LOCK, 5000)) {
            //
            // Copy the GROUPFILE structure to the shared memory block
            // after locking, since the open call will overwrite it.
            // The lpInternal and lpParent members must not be changed!
            //
            CopyMemory(memory->block, groupFile, offsetof(GROUPFILE, lpInternal));

            // Unlock will update the group-file.
            Io_ShmQuery(memory, DC_GROUPFILE_UNLOCK, 5000);

            status = TRUE;
            DebugPrint("Io_GroupSetFile: OKAY\n");
        } else {
            DebugPrint("Io_GroupSetFile: LOCK FAIL\n");
        }

        // Close the group-file before returning.
        Io_ShmQuery(memory, DC_GROUPFILE_CLOSE, 5000);
    } else {
        DebugPrint("Io_GroupSetFile: OPEN FAIL\n");
    }

    return status;
}

/*++

Io_GroupIdToName

    Resolves a group ID to its corresponding group name.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    groupId     - The group ID to resolve.

    groupName   - Pointer to a buffer to receive the user name. The
                  buffer must be able to hold _MAX_NAME+1 characters.

Return Value:
    A standard boolean result.

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

    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    DebugPrint("Io_GroupIdToName: groupId=%d groupName=0x%p\n", groupId, groupName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    dcNameId->Id = groupId;

    if (!Io_ShmQuery(memory, DC_GID_TO_GROUP, 5000)) {
        StringCchCopyA(groupName, _MAX_NAME+1, dcNameId->tszName);

        DebugPrint("Io_GroupIdToName: OKAY\n");
        return TRUE;
    }

    groupName[0] = '\0';
    DebugPrint("Io_GroupIdToName: FAIL\n");
    return FALSE;
}

/*++

Io_GroupNameToId

    Resolves a group name to its corresponding group ID.

Arguments:
    memory      - Pointer to an allocated IO_MEMORY structure. The buffer
                  size must be large enough to hold the DC_NAMEID structure.

    groupName   - The group name to resolve.

    groupId     - Location to store the group ID.

Return Value:
    A standard boolean result.

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

    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    assert(groupId   != NULL);
    DebugPrint("Io_GroupNameToId: groupName=%s groupId=0x%p\n", groupName, groupId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    result = Io_ShmQuery(memory, DC_GROUP_TO_GID, 5000);
    if (result != (DWORD)-1) {
        *groupId = (int)result;
        DebugPrint("Io_GroupNameToId: OKAY\n");
        return TRUE;
    }

    *groupId = -1;
    DebugPrint("Io_GroupNameToId: FAIL\n");
    return FALSE;
}
