/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2008 Alcoholicz Scripting Team

Module Name:
    ioFTPD Shared Memory

Author:
    neoxed (neoxed@gmail.com) Mar 17, 2006

Abstract:
    ioFTPD shared memory function definitions.

--*/

#ifndef _ALCOWINIOSHM_H_
#define _ALCOWINIOSHM_H_

//
// Shared memory functions.
//

typedef struct {
    HWND  messageWnd;       // Handle to ioFTPD's message window.
    DWORD processId;        // Current process ID.
} ShmSession;

typedef struct {
    DC_MESSAGE *message;    // Data-copy message structure.
    void       *block;      // Allocated memory block.
    void       *remote;     // ???
    HANDLE     event;       // Event handle.
    HANDLE     memMap;      // Memory mapping handle.
    DWORD      bytes;       // Size of the memory block, in bytes.
} ShmMemory;

int
ShmInit(
    Tcl_Interp *interp,
    Tcl_Obj *windowObj,
    ShmSession *session
    );

ShmMemory *
ShmAlloc(
    Tcl_Interp *interp,
    ShmSession *session,
    DWORD bytes
    );

void
ShmFree(
    ShmSession *session,
    ShmMemory *memory
    );

DWORD
ShmQuery(
    ShmSession *session,
    ShmMemory *memory,
    DWORD queryType,
    DWORD timeOut
    );


//
// User and group functions.
//

int
UserCreate(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    int *userId
    );

int
UserRename(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    const char *newName
    );

int
UserDelete(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName
    );

int
UserGetFile(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    USERFILE *userFile
    );

int
UserSetFile(
    ShmSession *session,
    ShmMemory *memory,
    const USERFILE *userFile
    );

int
UserIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    char *userName
    );

int
UserNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    int *userId
    );

int
GroupCreate(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    int *groupId
    );

int
GroupRename(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    const char *newName
    );

int
GroupDelete(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName
    );

int
GroupGetFile(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    GROUPFILE *groupFile
    );

int
GroupSetFile(
    ShmSession *session,
    ShmMemory *memory,
    const GROUPFILE *groupFile
    );

int
GroupIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    char *groupName
    );

int
GroupNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    int *groupId
    );


//
// VFS functions.
//

typedef struct {
    UINT32 userId;
    UINT32 groupId;
    DWORD  fileMode;
} VfsPerm;

int
VfsFlush(
    ShmSession *session,
    ShmMemory *memory,
    const char *dirPath
    );

int
VfsRead(
    ShmSession *session,
    ShmMemory *memory,
    const char *path,
    int pathLength,
    VfsPerm *vfsPerm
    );

int
VfsWrite(
    ShmSession *session,
    ShmMemory *memory,
    const char *path,
    int pathLength,
    const VfsPerm *vfsPerm
    );

#endif // _ALCOWINIOSHM_H_
