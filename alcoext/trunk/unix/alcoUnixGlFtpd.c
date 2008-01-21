/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2008 Alcoholicz Scripting Team

Module Name:
    glFTPD

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Implements a Tcl command-based interface for interaction with glFTPD.

    glftpd open <shmKey>
      - Returns a glFTPD session handle, used by all subcommands.
      - The default "etc" directory path and version is "/glftpd/etc" and
        2.01, respectively. These default values can be changed with the
        "glftpd config" command.

    glftpd config <handle> [<switch> [value] ...]
      - Modify and retrieve options for glFTPD handles.
      - This command behaves similar to Tcl's "fconfigure" command. If no
        switches are given, a list of options and values is returned. If
        only a switch is given, its value is returned. Multiple pairs of
        switches and values can be given to modify handle options.
      - Switches:
         -etc [path]    - Path to glFTPD's "etc" directory.
         -key [shmKey]  - Shared memory key.
         -version [ver] - glFTPD online structure version, must be 1.3, 2.00, or 2.01.

    glftpd close <handle>
      - Closes the given glFTPD handle.

    glftpd info handles
      - List all open glFTPD handles.

    glftpd info maxusers <handle>
      - Retrieves the maximum number of simultaneous users.

    glftpd kill <handle> <process id>
      - Kills the specified glFTPD session.
      - An error is raised if the given process ID does not belong to glFTPD
        or does not exist.

    glftpd who <handle> <fields>
      - Retrieves online user information from glFTPD's shared memory segment.
      - Fields: action, gid, group, host, idletime, logintime, path,
                pid, size, speed, ssl, status, tagline, uid, or user.

--*/

#include <alcoExt.h>

//
// Tcl command functions.
//

static int
GlOpenCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
GlConfigCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
GlCloseCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
GlInfoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
GlKillCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr);

static int
GlWhoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr);

//
// Online data functions.
//

static int
GetOnlineData(
    Tcl_Interp *interp,
    key_t shmKey,
    int version,
    int *maxUsers,
    GlOnlineGeneric ***onlineData
    );

static void
FreeOnlineData(
    int maxUsers,
    GlOnlineGeneric **onlineData
    );

static int
GetOnlineFields(
    Tcl_Interp *interp,
    GlHandle *handlePtr,
    unsigned char *fields,
    int fieldCount,
    GlNames **userListPtr,
    GlNames **groupListPtr
    );

//
// User and group functions.
//

inline int
ParseFields(
    const char *line,
    int delims,
    int *lengthPtr,
    int32_t *idPtr
    );

static int
GetNameList(
    Tcl_Interp *interp,
    const char *etcPath,
    const char *fileName,
    int delims,
    GlNames **listPtr
    );

static int32_t
GetIdFromName(
    GlNames **listPtr,
    const char *name
    );

static const char *
GetNameFromId(
    GlNames **listPtr,
    int32_t id
    );

static void
FreeNameList(
    GlNames **listPtr
    );


// Changes to this array must also be reflected in GetOnlineData().
static const GlVersion versions[] = {
    {"1.3",  sizeof(GlOnline130)},
    {"2.00", sizeof(GlOnline200)},
    {"2.01", sizeof(GlOnline201)},
    {NULL}
};

enum {
    GLFTPD_130 = 0,
    GLFTPD_200,
    GLFTPD_201
};

static const char *whoFields[] = {
    "action",
    "gid",
    "group",
    "host",
    "idletime",
    "logintime",
    "path",
    "pid",
    "size",
    "speed",
    "ssl",
    "status",
    "tagline",
    "uid",
    "user",
    NULL
};

enum {
    WHO_ACTION = 0,
    WHO_GID,
    WHO_GROUP,
    WHO_HOST,
    WHO_IDLETIME,
    WHO_LOGINTIME,
    WHO_PATH,
    WHO_PID,
    WHO_SIZE,
    WHO_SPEED,
    WHO_SSL,
    WHO_STATUS,
    WHO_TAGLINE,
    WHO_UID,
    WHO_USER
};


/*++

ParseFields

    Parse the name and ID fields from a "passwd" or "group" file entry.

Arguments:
    line      - Entry to parse.

    delims    - Number of required delimiters.

    lengthPtr - Pointer to receive the length of the first field.

    idPtr     - Pointer to receive the ID field.

Return Value:
    A standard Tcl result.

--*/
inline int
ParseFields(
    const char *line,
    int delims,
    int *lengthPtr,
    int32_t *idPtr
    )
{
    char *p = (char *)line;
    int i;

    *lengthPtr = 0;
    *idPtr = -1;

    for (i = 0; i < delims; i++) {
        if ((p = strchr(p, ':')) == NULL) {
            break;
        }
        p++;

        // Format: <name>:ignored:<ID>
        if (i == 0) {
            // Length of the "name" field in characters.
            *lengthPtr = (int)(p - line) - 1;

        } else if (i == 1) {
            // Retrieve the long value of the "ID" field.
            *idPtr = (int32_t)strtol(p, NULL, 10);
        }
    }

    return (i < delims || *lengthPtr < 1) ? TCL_ERROR : TCL_OK;
}

/*++

GetNameList

    Creates a list of names from the specified file located in "etcPath".

Arguments:
    interp      - Interpreter to use for error reporting.

    etcPath     - Path to glFTPD's "etc" directory.

    fileName    - File name located in "etcPath".

    delims      - Number of required delimiters.

    listPtr     - Pointer to a receive a list of "GlNames" structures.

Return Value:
    A standard Tcl result.

Remarks:
    If the function fails, an error message is left in the interpreter's result.

--*/
static int
GetNameList(
    Tcl_Interp *interp,
    const char *etcPath,
    const char *fileName,
    int delims,
    GlNames **listPtr
    )
{
    char *p;
    char line[512];
    char path[PATH_MAX];
    int nameLength;
    int32_t id;
    FILE *stream;

    strncpy(path, etcPath, ARRAYSIZE(path));
    strncat(path, fileName, ARRAYSIZE(path));
    path[ARRAYSIZE(path)-1] = '\0';

    stream = fopen(path, "r");
    if (stream == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to open \"", path, "\": ",
            Tcl_PosixError(interp), NULL);
        return TCL_ERROR;
    }

    while ((p = fgets(line, ARRAYSIZE(line), stream)) != NULL) {
        // Strip leading spaces and skip empty or commented lines.
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0' || *p == '#') {
            continue;
        }

        // File formats:
        // passwd (6 delims) - User:Password:UID:GID:Date:HomeDir:Irrelevant
        // group  (3 delims) - Group:Description:GID:Irrelevant
        if (ParseFields(p, delims, &nameLength, &id) == TCL_OK) {
            GlNames *current = (GlNames *)ckalloc(sizeof(GlNames));

            if (nameLength >= ARRAYSIZE(current->name)) {
                nameLength = ARRAYSIZE(current->name);
            } else {
                nameLength++;
            }
            strncpy(current->name, p, nameLength);
            current->name[nameLength-1] = '\0';
            current->id = id;

            // Insert entry at the list head.
            current->next = *listPtr;
            *listPtr = current;
        }
    }

    fclose(stream);
    return TCL_OK;
}

/*++

GetIdFromName

    Retrieves the ID for a given name.

Arguments:
    listPtr - Pointer to a "GlNames" structure that represents the list head.

    name    - The name to look-up.

Return Value:
    If the function is successful, the corresponding ID for the name is
    returned. If the function fails, -1 is returned.

--*/
static int32_t
GetIdFromName(
    GlNames **listPtr,
    const char *name
    )
{
    GlNames *current;

    for (current = *listPtr; current != NULL; current = current->next) {
        if (strcmp(name, current->name) == 0) {
            return current->id;
        }
    }
    return -1;
}

/*++

GetNameFromId

    Retrieves the name for a ID.

Arguments:
    listPtr - Pointer to a "GlNames" structure that represents the list head.

    id      - The ID to look-up.

Return Value:
    If the function is successful, the corresponding name for the ID is
    returned. If the function fails, an empty string is returned.

--*/
static const char *
GetNameFromId(
    GlNames **listPtr,
    int32_t id
    )
{
    GlNames *current;

    for (current = *listPtr; current != NULL; current = current->next) {
        if (id == current->id) {
            return current->name;
        }
    }
    return "";
}

/*++

FreeNameList

    Frees a list of "GlNames" structures.

Arguments:
    listPtr - Pointer to a "GlNames" structure that represents the list head.

Return Value:
    None.

--*/
static void
FreeNameList(
    GlNames **listPtr
    )
{
    GlNames *current;

    while (*listPtr != NULL) {
        current = (*listPtr)->next;
        ckfree((char *) *listPtr);
        *listPtr = current;
    }
}

/*++

GetOnlineData

    Retrieve online data from a glFTPD shared memory segment.

Arguments:
    interp        - Interpreter to use for error reporting.

    shmKey        - Shared memory key used by glFTPD.

    version       - Online structure version, must be an index in the "versions" array.

    maxUsers      - Location to store the maximum number of online users is stored.

    onlineDataPtr - Location to store the online data is stored. If this argument
                    is NULL, no data is allocated. The caller MUST free this data
                    when finished.

Return Value:
    A standard Tcl result.

Remarks:
    If the function fails, an error message is left in the interpreter's result.

--*/
static int
GetOnlineData(
    Tcl_Interp *interp,
    key_t shmKey,
    int version,
    int *maxUsers,
    GlOnlineGeneric ***onlineDataPtr
    )
{
    int i;
    int shmId;
    struct shmid_ds shmInfo;
    void *shmData;
    GlOnlineGeneric *entry;

    shmId = shmget(shmKey, 0, 0);
    if (shmId < 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to retrieve shared memory identifier: ",
            Tcl_PosixError(interp), NULL);
        return TCL_ERROR;
    }

    if (shmctl(shmId, IPC_STAT, &shmInfo) < 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to query shared memory segment: ",
            Tcl_PosixError(interp), NULL);
        return TCL_ERROR;
    }

    shmData = shmat(shmId, NULL, SHM_RDONLY);
    if (shmData == (void *) -1) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to retrieve shared memory data: ",
            Tcl_PosixError(interp), NULL);
        return TCL_ERROR;
    }

    if ((size_t)shmInfo.shm_segsz % versions[version].structSize) {
        Tcl_ResetResult(interp);
        Tcl_SetResult(interp, "unable to retrieve shared memory data: "
            "glftpd version mismatch", TCL_STATIC);
        return TCL_ERROR;
    }

    *maxUsers = (size_t)shmInfo.shm_segsz / versions[version].structSize;

    if (onlineDataPtr == NULL) {
        // Only the max user count was requested.
        return TCL_OK;
    }

    // Copy data into the generic online structure.
    *onlineDataPtr = (GlOnlineGeneric **)ckalloc(sizeof(GlOnlineGeneric *) * (*maxUsers));

    switch (version) {
        case GLFTPD_130: {
            GlOnline130 *glData = (GlOnline130 *)shmData;

            for (i = 0; i < *maxUsers; i++) {
                entry = (GlOnlineGeneric *)ckalloc(sizeof(GlOnlineGeneric));

                memcpy(entry->tagline  ,  glData[i].tagline,    sizeof(glData[i].tagline));
                memcpy(entry->username,   glData[i].username,   sizeof(glData[i].username));
                memcpy(entry->status,     glData[i].status,     sizeof(glData[i].status));
                memcpy(entry->host,       glData[i].host,       sizeof(glData[i].host));
                memcpy(entry->currentdir, glData[i].currentdir, sizeof(glData[i].currentdir));
                entry->ssl_flag      = -1; // Not present in glFTPD 1.3x.
                entry->groupid       = glData[i].groupid;
                entry->login_time    = glData[i].login_time;
                entry->tstart        = glData[i].tstart;
                entry->txfer.tv_sec  = 0; // Not present in glFTPD 1.3x.
                entry->txfer.tv_usec = 0;
                entry->bytes_xfer    = glData[i].bytes_xfer;
                entry->bytes_txfer   = 0; // Not present in glFTPD 1.3x.
                entry->procid        = glData[i].procid;
                (*onlineDataPtr)[i]  = entry;
            }
        }
        case GLFTPD_200: {
            GlOnline200 *glData = (GlOnline200 *)shmData;

            for (i = 0; i < *maxUsers; i++) {
                entry = (GlOnlineGeneric *)ckalloc(sizeof(GlOnlineGeneric));

                memcpy(entry->tagline  ,  glData[i].tagline,    sizeof(glData[i].tagline));
                memcpy(entry->username,   glData[i].username,   sizeof(glData[i].username));
                memcpy(entry->status,     glData[i].status,     sizeof(glData[i].status));
                memcpy(entry->host,       glData[i].host,       sizeof(glData[i].host));
                memcpy(entry->currentdir, glData[i].currentdir, sizeof(glData[i].currentdir));
                entry->ssl_flag     = glData[i].ssl_flag;
                entry->groupid      = glData[i].groupid;
                entry->login_time   = glData[i].login_time;
                entry->tstart       = glData[i].tstart;
                entry->txfer        = glData[i].txfer;
                entry->bytes_xfer   = glData[i].bytes_xfer;
                entry->bytes_txfer  = 0; // Not present in glFTPD 2.00.
                entry->procid       = glData[i].procid;
                (*onlineDataPtr)[i] = entry;
            }
        }
        case GLFTPD_201: {
            //
            // The "GlOnlineGeneric" structure is the exact same
            // as the "GlOnline201" structure (for now anyway).
            //
            assert(sizeof(GlOnlineGeneric) == sizeof(GlOnline201));

            for (i = 0; i < *maxUsers; i++) {
                entry = (GlOnlineGeneric *)ckalloc(sizeof(GlOnlineGeneric));
                memcpy(entry, shmData + (i * sizeof(GlOnline201)), sizeof(GlOnline201));
                (*onlineDataPtr)[i] = entry;
            }
        }
    }

    return TCL_OK;
}

/*++

FreeOnlineData

    Frees online data allocated by GetOnlineData().

Arguments:
    maxUsers      - Maximum number of online users.

    onlineDataPtr - Location of the online data.

Return Value:
    None.

--*/
static void
FreeOnlineData(
    int maxUsers,
    GlOnlineGeneric **onlineDataPtr
    )
{
    int i;
    for (i = 0; i < maxUsers; i++) {
        ckfree((char *)onlineDataPtr[i]);
    }
    ckfree((char *)onlineDataPtr);
}

/*++

GlOpenCmd

    Creates a new glFTPD session handle.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
GlOpenCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    char *etcPath;
    char handleName[6 + (sizeof(void*) * 2) + 3]; // Handle name, pointer in hex, and a NULL.
    int etcLength;
    int newEntry;
    long shmKey;
    GlHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "shmKey");
        return TCL_ERROR;
    }

    if (Tcl_GetLongFromObj(interp, objv[2], &shmKey) != TCL_OK) {
        return TCL_ERROR;
    }

    handlePtr = (GlHandle *)ckalloc(sizeof(GlHandle));

    etcPath   = GLFTPD_ETC_PATH;
    etcLength = strlen(etcPath) + 1;

    handlePtr->etcPath = ckalloc(etcLength);
    strncpy(handlePtr->etcPath, etcPath, etcLength);
    handlePtr->etcPath[etcLength-1] = '\0';

    handlePtr->shmKey  = (key_t) shmKey;
    handlePtr->version = GLFTPD_201;

    // Create a hash table entry and return the handle's identifier.
    snprintf(handleName, ARRAYSIZE(handleName), "glftpd%p", handlePtr);
    handleName[ARRAYSIZE(handleName)-1] = '\0';

    hashEntryPtr = Tcl_CreateHashEntry(statePtr->glftpdTable, handleName, &newEntry);
    if (newEntry == 0) {
        Tcl_Panic("Duplicate glftpd handle identifiers.");
    }
    Tcl_SetHashValue(hashEntryPtr, (ClientData)handlePtr);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), handleName, -1);
    return TCL_OK;
}

/*++

GlConfigCmd

    Changes options for an existing glFTPD session handle.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
GlConfigCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    int i;
    int index;
    GlHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;
    static const char *switches[] = {
        "-etc", "-key", "-version", NULL
    };
    enum switches {
        SWITCH_ETC = 0, SWITCH_KEY, SWITCH_VERSION
    };

    if (objc < 3 || (!(objc & 1) && objc != 4)) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle ?switch? ?value? ?switch value?...");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->glftpdTable, "glftpd");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (GlHandle *)Tcl_GetHashValue(hashEntryPtr);

    //
    // List all options and their corresponding values.
    // Cmd: glftpd config <handle>
    //
    if (objc == 3) {
        Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);

        for (i = 0; switches[i] != NULL; i++) {
            Tcl_ListObjAppendElement(NULL, resultPtr, Tcl_NewStringObj(switches[i], -1));

            switch ((enum switches) i) {
                case SWITCH_ETC: {
                    Tcl_ListObjAppendElement(NULL, resultPtr,
                        Tcl_NewStringObj(handlePtr->etcPath, -1));
                    break;
                }
                case SWITCH_KEY: {
                    Tcl_ListObjAppendElement(NULL, resultPtr,
                        Tcl_NewLongObj((long)handlePtr->shmKey));
                    break;
                }
                case SWITCH_VERSION: {
                    Tcl_ListObjAppendElement(NULL, resultPtr,
                        Tcl_NewStringObj(versions[handlePtr->version].name, -1));
                    break;
                }
            }
        }

        return TCL_OK;
    }

    //
    // Retrieve the value of a given option.
    // Cmd: glftpd config <handle> -switch
    //
    if (objc == 4) {
        if (Tcl_GetIndexFromObj(interp, objv[3], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }

        switch ((enum switches) index) {
            case SWITCH_ETC: {
                Tcl_SetStringObj(Tcl_GetObjResult(interp), handlePtr->etcPath, -1);
                break;
            }
            case SWITCH_KEY: {
                Tcl_SetLongObj(Tcl_GetObjResult(interp), (long)handlePtr->shmKey);
                break;
            }
            case SWITCH_VERSION: {
                Tcl_SetStringObj(Tcl_GetObjResult(interp), versions[handlePtr->version].name, -1);
                break;
            }
        }

        return TCL_OK;
    }

    //
    // Change one or more options.
    // Cmd: glftpd config <handle> -switch value
    // Cmd: glftpd config <handle> -switch value -switch value
    //
    for (i = 3; i < objc; i++) {
        char *name = Tcl_GetString(objv[i]);

        if (name[0] != '-') {
            break;
        }

        if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }

        i++;
        switch ((enum switches) index) {
            case SWITCH_ETC: {
                int etcLength;
                char *etcPath;

                ckfree(handlePtr->etcPath);
                etcPath = Tcl_GetStringFromObj(objv[i], &etcLength);

                etcLength++; // Accommodate for the terminating NULL.
                handlePtr->etcPath = ckalloc(etcLength);
                strncpy(handlePtr->etcPath, etcPath, etcLength);
                handlePtr->etcPath[etcLength-1] = '\0';
                break;
            }
            case SWITCH_KEY: {
                long shmKey;

                if (Tcl_GetLongFromObj(interp, objv[i], &shmKey) != TCL_OK) {
                    return TCL_ERROR;
                }
                handlePtr->shmKey = (key_t) shmKey;
                break;
            }
            case SWITCH_VERSION: {
                int version;

                if (Tcl_GetIndexFromObjStruct(interp, objv[i], versions,
                        sizeof(GlVersion), "version", TCL_EXACT, &version) != TCL_OK) {
                    return TCL_ERROR;
                }
                handlePtr->version = version;
                break;
            }
        }
    }

    return TCL_OK;
}

/*++

GlCloseCmd

    Closes a glFTPD session handle.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
GlCloseCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    GlHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->glftpdTable, "glftpd");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (GlHandle *)Tcl_GetHashValue(hashEntryPtr);

    // Free the handle structure and remove the hash table entry.
    ckfree(handlePtr->etcPath);
    ckfree((char *)handlePtr);
    Tcl_DeleteHashEntry(hashEntryPtr);

    return TCL_OK;
}

/*++

GlCloseHandles

    Closes all glFTPD session handles in the given hash table.

Arguments:
    tablePtr - Hash table of glFTPD session.

Return Value:
    None.

--*/
void
GlCloseHandles(
    Tcl_HashTable *tablePtr
    )
{
    GlHandle *handlePtr;
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    for (entryPtr = Tcl_FirstHashEntry(tablePtr, &search);
            entryPtr != NULL;
            entryPtr = Tcl_NextHashEntry(&search)) {

        handlePtr = (GlHandle *)Tcl_GetHashValue(entryPtr);
        ckfree(handlePtr->etcPath);
        ckfree((char *)handlePtr);
        Tcl_DeleteHashEntry(entryPtr);
    }
}

/*++

GlInfoCmd

    Retrieves information about a glFTPD session handle.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
GlInfoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    int index;
    Tcl_Obj *resultPtr;
    static const char *options[] = {
        "handles", "maxusers", NULL
    };
    enum optionIndices {
        OPTION_HANDLES = 0, OPTION_MAXUSERS
    };

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "option ?arg...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    resultPtr = Tcl_GetObjResult(interp);

    switch ((enum optionIndices) index) {
        case OPTION_HANDLES: {
            char *name;
            Tcl_HashEntry *hashEntryPtr;
            Tcl_HashSearch hashSearch;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 3, objv, NULL);
                return TCL_ERROR;
            }

            // Create a list of open glFTPD handles.
            for (hashEntryPtr = Tcl_FirstHashEntry(statePtr->glftpdTable, &hashSearch);
                    hashEntryPtr != NULL;
                    hashEntryPtr = Tcl_NextHashEntry(&hashSearch)) {

                name = Tcl_GetHashKey(statePtr->glftpdTable, hashEntryPtr);
                Tcl_ListObjAppendElement(NULL, resultPtr, Tcl_NewStringObj(name, -1));
            }
            return TCL_OK;
        }
        case OPTION_MAXUSERS: {
            int maxUsers;
            GlHandle *handlePtr;
            Tcl_HashEntry *hashEntryPtr;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 3, objv, "handle");
                return TCL_ERROR;
            }

            hashEntryPtr = GetHandleTableEntry(interp, objv[3], statePtr->glftpdTable, "glftpd");
            if (hashEntryPtr == NULL) {
                return TCL_ERROR;
            }
            handlePtr = (GlHandle *)Tcl_GetHashValue(hashEntryPtr);

            if (GetOnlineData(interp, handlePtr->shmKey, handlePtr->version, &maxUsers, NULL) != TCL_OK) {
                return TCL_ERROR;
            }

            Tcl_SetLongObj(resultPtr, (long)maxUsers);
            return TCL_OK;
        }
    }

    // This point is never reached.
    assert(0);
    return TCL_ERROR;
}

/*++

GlKillCmd

    Kills the specified glFTPD process ID.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
GlKillCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    int i;
    int maxUsers;
    int status = TCL_ERROR;
    long procId;
    GlHandle *handlePtr;
    GlOnlineGeneric **onlineData;
    Tcl_HashEntry *hashEntryPtr;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle pid");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->glftpdTable, "glftpd");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (GlHandle *)Tcl_GetHashValue(hashEntryPtr);

    if (Tcl_GetLongFromObj(interp, objv[3], &procId) != TCL_OK) {
        return TCL_ERROR;
    }

    if (GetOnlineData(interp, handlePtr->shmKey, handlePtr->version, &maxUsers, &onlineData) != TCL_OK) {
        return TCL_ERROR;
    }

    for (i = 0; i < maxUsers; i++) {
        if (onlineData[i]->procid > 0 && (pid_t)onlineData[i]->procid == (pid_t)procId) {

            if (kill((pid_t)onlineData[i]->procid, SIGTERM) == 0) {
                status = TCL_OK;
            } else {
                Tcl_AppendResult(interp, "unable to kill user: ",
                    Tcl_PosixError(interp), NULL);
            }
            goto end;
        }
    }

    //
    // This point is only reached if the given process ID
    // does not belong to glFTPD or it does not exist.
    //
    Tcl_SetResult(interp, "unable to kill user: the specified process "
        "does not belong to glFTPD or does not exist", TCL_STATIC);

end:
    FreeOnlineData(maxUsers, onlineData);
    return status;
}

/*++

GlWhoCmd

    Retrieves online user information from glFTPD's shared memory segment.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
GlWhoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    int elementCount;
    int fieldIndex;
    int i;
    int result = TCL_ERROR;
    unsigned char *fields;
    GlHandle *handlePtr;
    GlNames *groupListPtr = NULL;
    GlNames *userListPtr = NULL;
    Tcl_HashEntry *hashEntryPtr;
    Tcl_Obj **elementObjs;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle fields");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->glftpdTable, "glftpd");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (GlHandle *)Tcl_GetHashValue(hashEntryPtr);

    if (Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementObjs) != TCL_OK) {
        return TCL_ERROR;
    }

    // Create an array of indices from "whoFields".
    fields = (unsigned char *)ckalloc(elementCount * sizeof(unsigned char));

    for (i = 0; i < elementCount; i++) {
        if (Tcl_GetIndexFromObj(interp, elementObjs[i], whoFields,
                "field", 0, &fieldIndex) != TCL_OK) {
            goto end;
        }

        if (fieldIndex == WHO_GROUP) {
            // Group ID to name resolving.
            if (groupListPtr == NULL && GetNameList(interp, handlePtr->etcPath,
                    GLFTPD_GROUP, 3, &groupListPtr) != TCL_OK) {
                goto end;
            }
        } else if (fieldIndex == WHO_UID) {
            // User name to ID resolving.
            if (userListPtr == NULL && GetNameList(interp, handlePtr->etcPath,
                    GLFTPD_PASSWD, 6, &userListPtr) != TCL_OK) {
                goto end;
            }
        }

        assert(fieldIndex < 255);
        fields[i] = (unsigned char)fieldIndex;
    }

    result = GetOnlineFields(interp, handlePtr, fields, elementCount, &userListPtr, &groupListPtr);

end:
    if (userListPtr != NULL) {
        FreeNameList(&userListPtr);
    }
    if (groupListPtr != NULL) {
        FreeNameList(&groupListPtr);
    }

    ckfree((char *)fields);
    return result;
}

/*++

GetOnlineFields

    Retrieve a list of online users and the requested fields.

Arguments:
    interp       - Current interpreter.

    handlePtr    - Pointer to a "GlHandle" structure.

    fields       - Array of fields to retrieve.

    fieldCount   - Number of fields given for the "fields" parameter.

    userListPtr  - Pointer to a "GlNames" structure that represents the
                   user list head.

    groupListPtr - Pointer to a "GlNames" structure that represents the
                   group list head.

Return Value:
    A standard Tcl result.

Remarks:
    If the function succeeds, the online list is left in the interpreter's
    result. If the function fails, an error message is left instead.

--*/
static int
GetOnlineFields(
    Tcl_Interp *interp,
    GlHandle *handlePtr,
    unsigned char *fields,
    int fieldCount,
    GlNames **userListPtr,
    GlNames **groupListPtr
    )
{
    int i;
    int j;
    int maxUsers;
    struct timeval timeNow;
    GlOnlineGeneric **onlineData;
    Tcl_Obj *fieldObj;
    Tcl_Obj *resultObj;
    Tcl_Obj *userObj;

    if (GetOnlineData(interp, handlePtr->shmKey, handlePtr->version, &maxUsers, &onlineData) != TCL_OK) {
        return TCL_ERROR;
    }
    gettimeofday(&timeNow, NULL);

    //
    // Create a nested list of users and requested fields.
    // Ex: {fieldOne fieldTwo ...} {fieldOne fieldTwo ...} {fieldOne fieldTwo ...}
    //
    resultObj = Tcl_GetObjResult(interp);

    for (i = 0; i < maxUsers; i++) {
        if (onlineData[i]->procid <= 0) {
            continue;
        }
        userObj = Tcl_NewObj();

        for (j = 0; j < fieldCount; j++) {
            fieldObj = NULL;

            switch ((int)fields[j]) {
                case WHO_ACTION: {
                    fieldObj = Tcl_NewStringObj(onlineData[i]->status, -1);
                    break;
                }
                case WHO_GID: {
                    fieldObj = Tcl_NewLongObj((long)onlineData[i]->groupid);
                    break;
                }
                case WHO_GROUP: {
                    fieldObj = Tcl_NewStringObj(GetNameFromId(groupListPtr,
                        onlineData[i]->groupid), -1);
                    break;
                }
                case WHO_HOST: {
                    fieldObj = Tcl_NewStringObj(onlineData[i]->host, -1);
                    break;
                }
                case WHO_IDLETIME: {
                    fieldObj = Tcl_NewLongObj((long)timeNow.tv_sec - (long)onlineData[i]->tstart.tv_sec);
                    break;
                }
                case WHO_LOGINTIME: {
                    fieldObj = Tcl_NewLongObj((long)onlineData[i]->login_time);
                    break;
                }
                case WHO_PATH: {
                    fieldObj = Tcl_NewStringObj(onlineData[i]->currentdir, -1);
                    break;
                }
                case WHO_PID: {
                    fieldObj = Tcl_NewLongObj((long)onlineData[i]->procid);
                    break;
                }
                case WHO_SIZE: {
                    fieldObj = Tcl_NewWideIntObj((Tcl_WideInt)onlineData[i]->bytes_xfer);
                    break;
                }
                case WHO_SPEED: {
                    double speed = (onlineData[i]->bytes_xfer / 1024.0) /
                        (((long)timeNow.tv_sec - (long)onlineData[i]->tstart.tv_sec) * 1.0 +
                        ((long)timeNow.tv_usec - (long)onlineData[i]->tstart.tv_usec) / 1000000.0);

                    fieldObj = Tcl_NewDoubleObj(speed);
                    break;
                }
                case WHO_SSL: {
                    fieldObj = Tcl_NewLongObj((long)onlineData[i]->ssl_flag);
                    break;
                }
                case WHO_STATUS: {
                    int status = 0;

                    //
                    // If a transfer completes and the user does not issue another
                    // command, the "status" field still contains RETR/STOR/APPE.
                    // In order to prevent this lingering effect, we check if any
                    // data has been transferred first.
                    //
                    if (onlineData[i]->bytes_xfer > 0) {
                        if (strncasecmp(onlineData[i]->status, "RETR ", 5) == 0) {
                            status = 1;
                        } else if (strncasecmp(onlineData[i]->status, "STOR ", 5) == 0 ||
                                   strncasecmp(onlineData[i]->status, "APPE ", 5) == 0) {
                            status = 2;
                        }
                    } else if (strncasecmp(onlineData[i]->status, "LIST ", 5) == 0 ||
                               strncasecmp(onlineData[i]->status, "NLST ", 5) == 0 ||
                               strncasecmp(onlineData[i]->status, "STAT ", 5) == 0) {
                        status = 3;
                    }

                    //
                    // 0 - Idle
                    // 1 - Download
                    // 2 - Upload
                    // 3 - Listing
                    //
                    fieldObj = Tcl_NewIntObj(status);
                    break;
                }
                case WHO_TAGLINE: {
                    fieldObj = Tcl_NewStringObj(onlineData[i]->tagline, -1);
                    break;
                }
                case WHO_UID: {
                    fieldObj = Tcl_NewLongObj((long)GetIdFromName(userListPtr,
                        onlineData[i]->username));
                    break;
                }
                case WHO_USER: {
                    fieldObj = Tcl_NewStringObj(onlineData[i]->username, -1);
                    break;
                }
            }

            assert(fieldObj != NULL);
            Tcl_ListObjAppendElement(NULL, userObj, fieldObj);
        }

        Tcl_ListObjAppendElement(NULL, resultObj, userObj);
    }

    FreeOnlineData(maxUsers, onlineData);
    return TCL_OK;
}


/*++

GlFtpdObjCmd

    This function provides the "glftpd" Tcl command.

Arguments:
    clientData - Pointer to a "ExtState" structure.

    interp     - Current interpreter.

    objc       - Number of arguments.

    objv       - Argument objects.

Return Value:
    A standard Tcl result.

--*/
int
GlFtpdObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    ExtState *statePtr = (ExtState *)clientData;
    int index;
    static const char *options[] = {
        "close", "config", "info",
        "kill", "open", "who", NULL
    };
    enum optionIndices {
        OPTION_CLOSE = 0, OPTION_CONFIG, OPTION_INFO,
        OPTION_KILL, OPTION_OPEN, OPTION_WHO
    };

    // Make sure the modified online structures are the exact same size as
    // the original data structure. This check is needed in case the Tcl
    // extension is built on a 64bit system.
    assert(sizeof(GlOnline130) == 880);
    assert(sizeof(GlOnline200) == 896);
    assert(sizeof(GlOnline201) == 904);

    // Validate "versions" indices.
    assert(!strcmp("1.3",  versions[GLFTPD_130].name));
    assert(!strcmp("2.00", versions[GLFTPD_200].name));
    assert(!strcmp("2.01", versions[GLFTPD_201].name));

    // Validate "whoFields" indices.
    assert(!strcmp("action",    whoFields[WHO_ACTION]));
    assert(!strcmp("gid",       whoFields[WHO_GID]));
    assert(!strcmp("group",     whoFields[WHO_GROUP]));
    assert(!strcmp("host",      whoFields[WHO_HOST]));
    assert(!strcmp("idletime",  whoFields[WHO_IDLETIME]));
    assert(!strcmp("logintime", whoFields[WHO_LOGINTIME]));
    assert(!strcmp("path",      whoFields[WHO_PATH]));
    assert(!strcmp("pid",       whoFields[WHO_PID]));
    assert(!strcmp("size",      whoFields[WHO_SIZE]));
    assert(!strcmp("speed",     whoFields[WHO_SPEED]));
    assert(!strcmp("ssl",       whoFields[WHO_SSL]));
    assert(!strcmp("status",    whoFields[WHO_STATUS]));
    assert(!strcmp("tagline",   whoFields[WHO_TAGLINE]));
    assert(!strcmp("uid",       whoFields[WHO_UID]));
    assert(!strcmp("user",      whoFields[WHO_USER]));

    // Check arguments.
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_CLOSE:  return GlCloseCmd(interp, objc, objv, statePtr);
        case OPTION_CONFIG: return GlConfigCmd(interp, objc, objv, statePtr);
        case OPTION_INFO:   return GlInfoCmd(interp, objc, objv, statePtr);
        case OPTION_KILL:   return GlKillCmd(interp, objc, objv, statePtr);
        case OPTION_OPEN:   return GlOpenCmd(interp, objc, objv, statePtr);
        case OPTION_WHO:    return GlWhoCmd(interp, objc, objv, statePtr);
    }

    // This point is never reached.
    assert(0);
    return TCL_ERROR;
}
