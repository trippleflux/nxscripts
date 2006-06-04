/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Group Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for group modules.

*/

#include "mydb.h"

//
// Function and type declarations
//

static BOOL  MODULE_CALL GroupFinalize(void);
static INT32 MODULE_CALL GroupCreate(char *groupName);
static BOOL  MODULE_CALL GroupRename(char *groupName, INT32 groupId, char *newName);
static BOOL  MODULE_CALL GroupDelete(char *groupName, INT32 groupId);
static BOOL  MODULE_CALL GroupLock(GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupUnlock(GROUPFILE *groupFile);
static INT   MODULE_CALL GroupRead(char *filePath, GROUPFILE *groupFile);
static INT   MODULE_CALL GroupWrite(GROUPFILE *groupFile);
static INT   MODULE_CALL GroupOpen(char *groupName, GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupClose(GROUPFILE *groupFile);

typedef struct {
    HANDLE fileHandle;
} GROUP_CONTEXT;

//
// Local variables
//

static GROUP_MODULE *groupModule = NULL;


INT
MODULE_CALL
GroupModuleInit(
    GROUP_MODULE *module
    )
{
    DebugPrint("GroupInit: module=%p\n", module);

    // Initialize module structure
    module->tszModuleName = "NXMYDB";
    module->DeInitialize  = GroupFinalize;
    module->Create        = GroupCreate;
    module->Delete        = GroupDelete;
    module->Rename        = GroupRename;
    module->Lock          = GroupLock;
    module->Write         = GroupWrite;
    module->Open          = GroupOpen;
    module->Close         = GroupClose;
    module->Unlock        = GroupUnlock;

    // Initialize procedure table
    if (!InitProcTable(module->GetProc)) {
        return 1;
    }
    Io_Putlog(LOG_ERROR, "nxMyDB group module loaded.\r\n");

    groupModule = module;
    return 0;
}

static
INT
MODULE_CALL
GroupFinalize(
    void
    )
{
    DebugPrint("GroupFinalize: module=%p\n", groupModule);
    Io_Putlog(LOG_ERROR, "nxMyDB group module unloaded.\r\n");

    // Finalize procedure table
    FinalizeProcTable();
    groupModule = NULL;
    return 0;
}

static
INT32
MODULE_CALL
GroupCreate(
    char *groupName
    )
{
    char buffer[_MAX_NAME + 12];
    char *sourcePath;
    char *tempPath;
    DWORD error;
    INT32 result;
    GROUPFILE groupFile;

    DebugPrint("GroupCreate: groupName=\"%s\"\n", groupName);

    // Retrieve default location
    sourcePath = Io_ConfigGetPath("Locations", "Group_Files", "Default.Group", NULL);
    if (sourcePath == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    // Retrieve temporary location
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%s.temp", groupName);
    tempPath = Io_ConfigGetPath("Locations", "Group_Files", buffer, NULL);
    if (tempPath == NULL) {
        Io_Free(sourcePath);

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    DebugPrint("GroupCreate: sourcePath=\"%s\" tempPath=\"%s\"\n", sourcePath, tempPath);

    // Copy default file
    if (!CopyFileA(sourcePath, tempPath, FALSE)) {
        error = GetLastError();

        // Free resources
        Io_Free(sourcePath);
        Io_Free(tempPath);

        // Restore system error code
        SetLastError(error);
        return -1;
    }

    // Initialize GROUPFILE structure
    ZeroMemory(&groupFile, sizeof(GROUPFILE));

    if (GroupRead(tempPath, &groupFile) != UM_SUCCESS) {
        error = GetLastError();

        // Free resources
        DeleteFileA(tempPath);
        Io_Free(sourcePath);
        Io_Free(tempPath);

        // Restore system error code
        SetLastError(error);
        return -1;
    }

    // Register group
    result = groupModule->Register(groupModule, groupName, &groupFile);
    if (result == -1) {
        error = GetLastError();

        GroupClose(&groupFile);
        DeleteFileA(tempPath);
    } else {
        char *offset;
        size_t sourceLength = strlen(sourcePath);

        // Terminate string after the last path separator
        offset = strrchr(sourcePath, '\\');
        if (offset != NULL) {
            offset[1] = '\0';
        } else {
            offset = strrchr(sourcePath, '/');
            if (offset != NULL) {
                offset[1] = '\0';
            }
        }

        // Append the group's ID
        StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", result);
        StringCchCatA(sourcePath, sourceLength, buffer);
        DebugPrint("GroupCreate:  finalPath=\"%s\"\n", sourcePath);

        // Rename temporary file
        if (!MoveFileExA(tempPath, sourcePath, MOVEFILE_REPLACE_EXISTING)) {
            error = GetLastError();

            // Unregister group and delete the data file
            if (!groupModule->Unregister(groupModule, groupName)) {
                GroupClose(&groupFile);
            }
            DeleteFileA(tempPath);

            result = -1;
        } else {
            error = ERROR_SUCCESS;
        }
    }

    // Free resources
    Io_Free(sourcePath);
    Io_Free(tempPath);

    // Restore system error code
    SetLastError(error);
    return result;
}

static
BOOL
MODULE_CALL
GroupRename(
    char *groupName,
    INT32 groupId,
    char *newName
    )
{
    DebugPrint("GroupRename: groupName=\"%s\" groupId=%i newName=\"%s\"\n", groupName, groupId, newName);
    return groupModule->RegisterAs(groupModule, groupName, newName);
}

static
BOOL
MODULE_CALL
GroupDelete(
    char *groupName,
    INT32 groupId
    )
{
    char buffer[16];
    char *filePath;

    DebugPrint("GroupDelete: groupName=\"%s\" groupId=%i\n", groupName, groupId);

    // Format group ID
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", groupId);

    // Retrieve file location
    filePath = Io_ConfigGetPath("Locations", "Group_Files", buffer, NULL);
    if (filePath == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrint("GroupDelete: Unable to retrieve file location.\n");
        return 1;
    }

    // Delete data file and free resources
    DeleteFileA(filePath);
    Io_Free(filePath);

    // Unregister group
    return groupModule->Unregister(groupModule, groupName);
}

static
BOOL
MODULE_CALL
GroupLock(
    GROUPFILE *groupFile
    )
{
    DebugPrint("GroupLock: groupFile=%p\n", groupFile);
    return 0;
}

static
BOOL
MODULE_CALL
GroupUnlock(
    GROUPFILE *groupFile
    )
{
    DebugPrint("GroupUnlock: groupFile=%p\n", groupFile);
    return 0;
}

static
INT
MODULE_CALL
GroupRead(
    char *filePath,
    GROUPFILE *groupFile
    )
{
    char *buffer;
    DWORD bytesRead;
    DWORD error;
    DWORD fileSize;
    INT result;
    GROUP_CONTEXT *context;

    DebugPrint("GroupRead: filePath=\"%s\" groupFile=%p\n", filePath, groupFile);

    // Allocate group context
    context = (GROUP_CONTEXT *)Io_Allocate(sizeof(GROUP_CONTEXT));
    if (context == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrint("GroupRead: Unable to allocate group context.\n");
        return UM_FATAL;
    }

    buffer = NULL;
    result = UM_FATAL;

    // Open the group's data file
    context->fileHandle = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (context->fileHandle == INVALID_HANDLE_VALUE) {
        result = (GetLastError() == ERROR_FILE_NOT_FOUND) ? UM_DELETED : UM_FATAL;
        DebugPrint("GroupRead: Unable to open file (error %lu).\n", GetLastError());
        goto end;
    }

    // Retrieve file size
    fileSize = GetFileSize(context->fileHandle, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize < 5) {
        DebugPrint("GroupRead: Unable to retrieve file size, or file size is under 5 bytes.\n");
        goto end;
    }

    // Allocate read buffer
    buffer = (char *)Io_Allocate(fileSize + 1);
    if (buffer == NULL) {
        DebugPrint("GroupRead: Unable to allocate read buffer.\n", GetLastError());
        goto end;
    }

    // Read data file to buffer
    if (!ReadFile(context->fileHandle, buffer, fileSize, &bytesRead, NULL) || bytesRead < 5) {
        DebugPrint("GroupRead: Unable to read file, or the amount read is under 5 bytes.\n");
        goto end;
    }

    // Pad buffer with a new-line
    buffer[bytesRead] = '\n';
    bytesRead++;

    // Parse buffer, initializing the GROUPFILE structure
    Io_Ascii2GroupFile(buffer, bytesRead, groupFile);
    groupFile->lpInternal  = context;
    result                 = UM_SUCCESS;

end:
    // Free objects and resources
    if (result != UM_SUCCESS) {
        error = GetLastError();
        if (context->fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(context->fileHandle);
        }
        Io_Free(context);
        Io_Free(buffer);

        // Restore system error code
        SetLastError(error);
    } else {
        Io_Free(buffer);
    }

    return result;
}

static
INT
MODULE_CALL
GroupWrite(
    GROUPFILE *groupFile
    )
{
    BUFFER buffer;
    DWORD bytesWritten;
    DWORD error;
    GROUP_CONTEXT *context;

    DebugPrint("GroupWrite: groupFile=%p\n", groupFile);

    // Retrieve group context
    context = (GROUP_CONTEXT *)groupFile->lpInternal;

    // Allocate write buffer
    buffer.size = 4096;
    buffer.len  = 0;
    buffer.buf  = (char *)Io_Allocate(buffer.size);
    if (buffer.buf == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 1;
    }

    // Dump group data to buffer
    Io_GroupFile2Ascii(&buffer, groupFile);

    // Write buffer to file
    SetFilePointer(context->fileHandle, 0, 0, FILE_BEGIN);
    if (!WriteFile(context->fileHandle, buffer.buf, buffer.len, &bytesWritten, NULL)) {
        error = GetLastError();
        Io_Free(buffer.buf);
        SetLastError(error);
        return 1;
    }

    // Truncate remaining data
    SetEndOfFile(context->fileHandle);

    Io_Free(buffer.buf);
    return 0;
}

static
INT
MODULE_CALL
GroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    char buffer[16];
    char *filePath;
    INT result;

    DebugPrint("GroupOpen: groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    // Format group ID
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", groupFile->Gid);

    // Retrieve file location
    filePath = Io_ConfigGetPath("Locations", "Group_Files", buffer, NULL);
    if (filePath == NULL) {
        DebugPrint("GroupOpen: Unable to retrieve file location.\n");
        return UM_FATAL;
    }

    // Read the group's data file
    result = GroupRead(filePath, groupFile);
    Io_Free(filePath);

    return result;
}

static
BOOL
MODULE_CALL
GroupClose(
    GROUPFILE *groupFile
    )
{
    BOOL result;
    GROUP_CONTEXT *context;

    DebugPrint("GroupClose: groupFile=%p\n", groupFile);

    // Retrieve group context
    context = (GROUP_CONTEXT *)groupFile->lpInternal;
    if (context == NULL) {
        return FALSE;
    }

    // Free objects and resources
    result = CloseHandle(context->fileHandle);
    Io_Free(context);

    return result;
}
