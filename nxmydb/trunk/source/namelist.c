/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Name List

Author:
    neoxed (neoxed@gmail.com) Sep 15, 2007

Abstract:
    Create and search a through an array of names.

*/

#include <base.h>
#include <array.h>
#include <namelist.h>

static INT CompareName(const VOID *elem1, const VOID *elem2)
{
    const NAME_ENTRY **entry1 = elem1;
    const NAME_ENTRY **entry2 = elem2;

    return strcmp(entry1[0]->name, entry2[0]->name);
}

DWORD FCALL NameListCreate(NAME_LIST *list, const CHAR *module, const CHAR *path)
{
    ASSERT(list != NULL);
    ASSERT(path != NULL);

    // TODO

    return ERROR_SUCCESS;
}

DWORD FCALL NameListCreateGroups(NAME_LIST *list, const CHAR *module)
{
    CHAR    *path;
    DWORD   result;

    ASSERT(list != NULL);

    path = Io_ConfigGet("Locations", "Group_Id_Table", NULL, NULL);
    if (path == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    result = NameListCreate(list, module, path);

    Io_Free(path);
    return result;
}

DWORD FCALL NameListCreateUsers(NAME_LIST *list, const CHAR *module)
{
    CHAR    *path;
    DWORD   result;

    ASSERT(list != NULL);

    path = Io_ConfigGet("Locations", "User_Id_Table", NULL, NULL);
    if (path == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    result = NameListCreate(list, module, path);

    Io_Free(path);
    return result;
}

DWORD FCALL NameListDestroy(NAME_LIST *list)
{
    ASSERT(list != NULL);

    if (list->array != NULL) {
        // Free all array entries
        while (list->count--) {
            Io_Free(list->array[list->count]);
        }
        Io_Free(list->array);
    }

    ZeroMemory(list, sizeof(NAME_LIST));
    return ERROR_SUCCESS;
}

BOOL FCALL NameListExists(NAME_LIST *list, const CHAR *name)
{
    NAME_ENTRY  *entry;
    NAME_ENTRY  **vector;

    ASSERT(list != NULL);
    ASSERT(name != NULL);

    // Move the "name" pointer by its offset in the NAME_ENTRY structure. This
    // is done instead of allocating stack space for a NAME_ENTRY structure and
    // copying the specified name into it.
    entry = (NAME_ENTRY *)((BYTE *)name - offsetof(NAME_ENTRY, name));

    // Search array for the entry
    vector = ArrayPtrSearch(&entry, list->array, list->count, CompareName);
    return (vector == NULL) ? FALSE : TRUE;
}

BOOL FCALL NameListRemove(NAME_LIST *list, const CHAR *name)
{
    NAME_ENTRY  *entry;
    NAME_ENTRY  **vector;

    ASSERT(list != NULL);
    ASSERT(name != NULL);

    // Move the "name" pointer by its offset in the NAME_ENTRY structure. This
    // is done instead of allocating stack space for a NAME_ENTRY structure and
    // copying the specified name into it.
    entry = (NAME_ENTRY *)((BYTE *)name - offsetof(NAME_ENTRY, name));

    // Search array for the entry
    vector = ArrayPtrSearch(&entry, list->array, list->count, CompareName);

    if (vector != NULL) {
        // Dereference pointer to get the actual NAME_ENTRY structure
        entry = vector[0];

        // Remove the entry from the array
        ArrayPtrDelete(&entry, list->array, list->count, CompareName);
        Io_Free(entry);

        // Decrement the element count
        --list->count;

        return TRUE;
    }

    return FALSE;
}
