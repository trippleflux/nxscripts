/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

Module Name:
    Name List

Author:
    neoxed (neoxed@gmail.com) Sep 15, 2007

Abstract:
    Create and search a through an array of names.

*/

#include <base.h>
#include <array.h>
#include <backends.h>
#include <namelist.h>

static INT CompareName(const VOID *elem1, const VOID *elem2)
{
    const NAME_ENTRY **entry1 = elem1;
    const NAME_ENTRY **entry2 = elem2;

    return strcmp(entry1[0]->name, entry2[0]->name);
}

static const CHAR *FindChar(CHAR ch, const CHAR *buffer, const CHAR *bufferEnd)
{
    ASSERT(buffer != NULL);
    ASSERT(bufferEnd != NULL);

    for (; buffer < bufferEnd; buffer++) {
        if (*buffer == ch) {
            return buffer;
        }
    }

    return NULL;
}

static INLINE DWORD TableRead(const CHAR *path, CHAR **buffer, SIZE_T *bufferLength)
{
    CHAR    *data;
    DWORD   dataLength;
    DWORD   bytesRead;
    DWORD   result;
    HANDLE  handle;

    ASSERT(path != NULL);
    ASSERT(buffer != NULL);
    ASSERT(bufferLength != NULL);

    handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    //  Get file size
    dataLength = GetFileSize(handle, NULL);
    if (dataLength == INVALID_FILE_SIZE) {
        result = GetLastError();
    } else {

        //  Allocate read buffer
        data = MemAllocate(dataLength);
        if (data == NULL) {
            result = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            //  Read from file
            if (ReadFile(handle, data, dataLength, &bytesRead, NULL) && bytesRead > 0) {
                *buffer       = data;
                *bufferLength = dataLength;
                result        = ERROR_SUCCESS;
            } else {
                result = GetLastError();
                MemFree(data);
            }
        }
    }

    CloseHandle(handle);
    return result;
}

static INLINE DWORD TableParseInsert(NAME_LIST *list, const CHAR *name, SIZE_T nameLength, INT32 id)
{
    NAME_ENTRY  *entry;
    NAME_ENTRY  **vector;
    VOID        *newMem;

    ASSERT(list != NULL);
    ASSERT(name != NULL);
    ASSERT(nameLength > 0);

    // Allocate and initialize entry structure
    entry = MemAllocate(sizeof(NAME_ENTRY));
    if (entry == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    entry->id = id;
    StringCchCopyNA(entry->name, ELEMENT_COUNT(entry->name), name, nameLength);

    if (list->count >= list->total) {
        // Increase the size of the array by 128 entries
        newMem = MemReallocate(list->array, (list->total + 128) * sizeof(NAME_ENTRY *));
        if (newMem == NULL) {
            MemFree(entry);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        list->array = newMem;
        list->total += 128;
    }

    // Insert entry into the array
    vector = ArrayPtrInsert(entry, list->array, list->count, CompareName);
    if (vector != NULL) {
        // Entry already exists
        MemFree(entry);
    } else {
        ++list->count;
    }

    return ERROR_SUCCESS;
}

static INLINE DWORD TableParse(NAME_LIST *list, const CHAR *buffer, SIZE_T bufferLength)
{
    const CHAR  *bufferEnd;
    const CHAR  *line;
    const CHAR  *lineEnd;
    const CHAR  *name;
    const CHAR  *module;
    CHAR        *stop;
    DWORD       result;
    INT32       id;
    SIZE_T      delims;
    SIZE_T      nameLength;
    SIZE_T      moduleLength;

    ASSERT(list != NULL);
    ASSERT(buffer != NULL);

    bufferEnd = &buffer[bufferLength];

    for (; buffer < bufferEnd; buffer++) {
        //
        // Format: <name>:<id>:<module>
        //
        //  name   - Name of the user, must not exceed _MAX_NAME.
        //  id     - Unique identifier of the user, a 32-bit signed integer.
        //  module - Name of module, must not exceed _MAX_NAME.
        //

        // Ignore EOL characters and white-space
        if (IS_EOL(*buffer) || IS_SPACE(*buffer)) {
            continue;
        }

        // Find the end of the current line
        line   = buffer;
        delims = 0;
        while (buffer < bufferEnd && !IS_EOL(*buffer)) {
            if (*buffer++ == ':') {
                delims++;
            }
        }
        if (delims != 2) {
            continue;
        }
        lineEnd = buffer;

        // Parse the name
        name       = line;
        line       = FindChar(':', line, lineEnd);
        nameLength = (line - name);
        if (nameLength == 0 || nameLength > _MAX_NAME) {
            continue;
        }
        line++;

        // Parse the ID
        id = strtoul(line, &stop, 10);
        if (id == ULONG_MAX || *stop != ':') {
            continue;
        }
        line = stop + 1;

        // Parse the module
        module       = line;
        moduleLength = (lineEnd - line);
#if 0
        if (moduleLength == 0 || moduleLength > _MAX_NAME) {
            continue;
        }
#else
        ASSERT(MODULE_NAME_LENGTH == strlen(MODULE_NAME));
        if (MODULE_NAME_LENGTH != moduleLength || memcmp(MODULE_NAME, module, moduleLength) != 0) {
            continue;
        }
#endif

        result = TableParseInsert(list, name, nameLength, id);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    }

    return ERROR_SUCCESS;
}


DWORD FCALL NameListCreate(NAME_LIST *list, const CHAR *path)
{
    CHAR    *buffer;
    DWORD   result;
    SIZE_T  bufferLength;

    ASSERT(list != NULL);
    ASSERT(path != NULL);

    // Initialize the NAME_LIST structure
    list->count = 0;
    list->total = 128;
    list->array = MemAllocate(list->total * sizeof(NAME_LIST *));
    if (list->array == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Buffer the ID table file
    result = TableRead(path, &buffer, &bufferLength);
    if (result == ERROR_SUCCESS) {

        // Parse the ID table file
        result = TableParse(list, buffer, bufferLength);

        MemFree(buffer);
    }

    if (result != ERROR_SUCCESS) {
        // Free all resources on failure
        NameListDestroy(list);
    }
    return result;
}

DWORD FCALL NameListCreateGroups(NAME_LIST *list)
{
    CHAR    *path;
    DWORD   result;

    ASSERT(list != NULL);

    path = Io_ConfigGet("Locations", "Group_Id_Table", NULL, NULL);
    if (path == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    result = NameListCreate(list, path);

    Io_Free(path);
    return result;
}

DWORD FCALL NameListCreateUsers(NAME_LIST *list)
{
    CHAR    *path;
    DWORD   result;

    ASSERT(list != NULL);

    path = Io_ConfigGet("Locations", "User_Id_Table", NULL, NULL);
    if (path == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    result = NameListCreate(list, path);

    Io_Free(path);
    return result;
}

DWORD FCALL NameListDestroy(NAME_LIST *list)
{
    ASSERT(list != NULL);

    if (list->array != NULL) {
        // Free all array entries
        while (list->count--) {
            MemFree(list->array[list->count]);
        }
        MemFree(list->array);
    }

    // Clear the structure
    ZeroMemory(list, sizeof(NAME_LIST));

    return ERROR_SUCCESS;
}

BOOL FCALL NameListExists(NAME_LIST *list, const CHAR *name)
{
    NAME_ENTRY  *entry;
    NAME_ENTRY  **vector;

    ASSERT(list != NULL);
    ASSERT(list->array != NULL);
    ASSERT(name != NULL);

    // Move the "name" pointer by its offset in the NAME_ENTRY structure. This
    // is done instead of allocating stack space for a NAME_ENTRY structure and
    // copying the specified name into it.
    entry = (NAME_ENTRY *)((BYTE *)name - offsetof(NAME_ENTRY, name));

    // Search array for the entry
    vector = ArrayPtrSearch(entry, list->array, list->count, CompareName);
    return (vector == NULL) ? FALSE : TRUE;
}

BOOL FCALL NameListRemove(NAME_LIST *list, const CHAR *name)
{
    NAME_ENTRY  *entry;
    NAME_ENTRY  **vector;

    ASSERT(list != NULL);
    ASSERT(list->array != NULL);
    ASSERT(name != NULL);

    // Move the "name" pointer by its offset in the NAME_ENTRY structure. This
    // is done instead of allocating stack space for a NAME_ENTRY structure and
    // copying the specified name into it.
    entry = (NAME_ENTRY *)((BYTE *)name - offsetof(NAME_ENTRY, name));

    // Search array for the entry
    vector = ArrayPtrSearch(entry, list->array, list->count, CompareName);

    if (vector != NULL) {
        // Deference pointer its overwritten during deletion
        entry = vector[0];

        // Remove entry from the array
        ArrayPtrDelete(vector, list->array, list->count, CompareName);

        // Decrement count and free entry
        --list->count;
        MemFree(entry);

        return TRUE;
    }

    return FALSE;
}
