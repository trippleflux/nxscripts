/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoUnixGlFtpd.c

Author:
    neoxed (neoxed@gmail.com) April 16, 2005

Abstract:
    Implements a Tcl command-based interface for interaction with glFTPD.

    glftpd open <shmKey>
      - Returns a glFTPD session handle, used by all subcommands.
      - The default 'etc' directory path and version is '/glftpd/etc' and
        2.01, respectively. These default values can be changed with the
        'glftpd config' command.

    glftpd config <handle> [<switch> [value] ...]
      - Modify and retrieve options for glFTPD handles.
      - This command behaves similar to Tcl's 'fconfigure' command. If no
        switches are given, a list of options and values is returned. If
        only a switch is given, its value is returned. Multiple pairs of
        switches and values can be given to modify handle options.
      - Switches:
         -etc [path]    - Path to glFTPD's 'etc' directory.
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
    GlUser **userListPtr,
    GlGroup **groupListPtr
    );

//
// User and group functions.
//

inline int
ParseFields(
    const char *line,
    int delims,
    int *lengthPtr,
    long *idPtr
    );

static int
GetUserList(
    Tcl_Interp *interp,
    const char *etcPath,
    GlUser **userListPtr
    );

static void
FreeUserList(
    GlUser **userListPtr
    );

static long
GetUserIdFromName(
    GlUser **userListPtr,
    const char *userName
    );

static int
GetGroupList(
    Tcl_Interp *interp,
    const char *etcPath,
    GlGroup **groupListPtr
    );

static void
FreeGroupList(
    GlGroup **groupListPtr
    );

static char *
GetGroupNameFromId(
    GlGroup **groupListPtr,
    long groupId
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

    Parse the name and ID fields from a 'passwd' or 'group' file entry.

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
    long *idPtr
    )
{
    char *p = (char *) line;
    int i;

    *lengthPtr = 0;
    for (i = 0; i < delims; i++) {
        if ((p = strchr(p, ':')) == NULL) {
            break;
        }
        p++;

        // Format: <name>:ignored:<ID>
        if (i == 0) {
            // Length of the 'name' field in characters.
            *lengthPtr = (int) (p - line) - 1;

        } else if (i == 1) {
            // Retrieve the long value of the 'ID' field.
            *idPtr = strtol(p, NULL, 10);
        }
    }

    return (i < delims || *lengthPtr < 1) ? TCL_ERROR : TCL_OK;
}

/*++

GetUserList

    Creates a list of users from the 'passwd' file located in 'etcPath'.

Arguments:
    interp      - Interpreter to use for error reporting.

    etcPath     - Path to glFTPD's 'etc' directory.

    userListPtr - Pointer to a receive a list of 'GlUser' structures.

Return Value:
    A standard Tcl result.

Remarks:
    If the function fails, an error message is left in the interpreter's result.

--*/
static int
GetUserList(
    Tcl_Interp *interp,
    const char *etcPath,
    GlUser **userListPtr
    )
{
    char *p;
    char line[512];
    char passwdFile[PATH_MAX];
    int nameLength;
    long userId;
    FILE *stream;

    strncpy(passwdFile, etcPath, ARRAYSIZE(passwdFile));
    strncat(passwdFile, GLFTPD_PASSWD, ARRAYSIZE(passwdFile));
    passwdFile[ARRAYSIZE(passwdFile)-1] = '\0';

    stream = fopen(passwdFile, "r");
    if (stream == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to open \"", passwdFile, "\": ",
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

        // A 'passwd' entry has 6 delimiters for 7 fields.
        // Format: User:Password:UID:GID:Date:HomeDir:Irrelevant
        if (ParseFields(p, 6, &nameLength, &userId) == TCL_OK) {
            GlUser *userPtr = (GlUser *) ckalloc(sizeof(GlUser));

            if (nameLength >= GL_USER_LENGTH) {
                nameLength = GL_USER_LENGTH;
            } else {
                nameLength++;
            }
            strncpy(userPtr->name, p, nameLength);
            userPtr->name[nameLength-1] = '\0';
            userPtr->id = userId;

            // Insert entry at the list head.
            userPtr->next = *userListPtr;
            *userListPtr = userPtr;
        }
    }

    fclose(stream);
    return TCL_OK;
}

/*++

FreeUserList

    Frees a list of 'GlUser' structures.

Arguments:
    userListPtr - Pointer to a 'GlUser' structure that represents the list head.

Return Value:
    None.

--*/
static void
FreeUserList(
    GlUser **userListPtr
    )
{
    GlUser *userPtr;

    while (*userListPtr != NULL) {
        userPtr = (*userListPtr)->next;
        ckfree((char *) *userListPtr);
        *userListPtr = userPtr;
    }
}

/*++

GetUserIdFromName

    Retrieves the user ID for a given user name.

Arguments:
    userListPtr - Pointer to a 'GlUser' structure that represents the list head.

    userName    - The user name to look-up.

Return Value:
    If the function is successful, the corresponding user ID for the given
    user name is returned. If the function fails, -1 is returned.

--*/
static long
GetUserIdFromName(
    GlUser **userListPtr,
    const char *userName
    )
{
    GlUser *userPtr;

    for (userPtr = *userListPtr; userPtr != NULL; userPtr = userPtr->next) {
        if (strcmp(userName, userPtr->name) == 0) {
            return userPtr->id;
        }
    }
    return -1;
}

/*++

GetGroupList

    Creates a list of groups from the 'group' file located in 'etcPath'.

Arguments:
    interp       - Interpreter to use for error reporting.

    etcPath      - Path to glFTPD's 'etc' directory.

    groupListPtr - Pointer to a receive a list of 'GlGroup' structures.

Return Value:
    A standard Tcl result.

Remarks:
    If the function fails, an error message is left in the interpreter's result.

--*/
static int
GetGroupList(
    Tcl_Interp *interp,
    const char *etcPath,
    GlGroup **groupListPtr
    )
{
    char *p;
    char line[512];
    char groupFile[PATH_MAX];
    int nameLength;
    long userId;
    FILE *stream;

    strncpy(groupFile, etcPath, ARRAYSIZE(groupFile));
    strncat(groupFile, GLFTPD_GROUP, ARRAYSIZE(groupFile));
    groupFile[ARRAYSIZE(groupFile)-1] = '\0';

    stream = fopen(groupFile, "r");
    if (stream == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to open \"", groupFile, "\": ",
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

        // A 'passwd' entry has 3 delimiters for 4 fields.
        // Format: Group:Description:GID:Irrelevant
        if (ParseFields(p, 3, &nameLength, &userId) == TCL_OK) {
            GlGroup *groupPtr = (GlGroup *) ckalloc(sizeof(GlUser));

            if (nameLength >= GL_GROUP_LENGTH) {
                nameLength = GL_GROUP_LENGTH;
            } else {
                nameLength++;
            }
            strncpy(groupPtr->name, p, nameLength);
            groupPtr->name[nameLength-1] = '\0';
            groupPtr->id = userId;

            // Insert entry at the list head.
            groupPtr->next = *groupListPtr;
            *groupListPtr = groupPtr;
        }
    }

    fclose(stream);
    return TCL_OK;
}

/*++

FreeGroupList

    Frees a list of 'GlGroup' structures.

Arguments:
    groupListPtr - Pointer to a 'GlGroup' structure that represents the list head.

Return Value:
    None.

--*/
static void
FreeGroupList(
    GlGroup **groupListPtr
    )
{
    GlGroup *groupPtr;

    while (*groupListPtr != NULL) {
        groupPtr = (*groupListPtr)->next;
        ckfree((char *) *groupListPtr);
        *groupListPtr = groupPtr;
    }
}

/*++

GetGroupNameFromId

    Retrieves the group's name for a given group ID.

Arguments:
    groupListPtr - Pointer to a 'GlGroup' structure that represents the list head.

    groupId      - The group ID to look-up.

Return Value:
    If the function is successful, the corresponding group name for the given
    group ID is returned. If the function fails, "NoGroup" is returned.

--*/
static char *
GetGroupNameFromId(
    GlGroup **groupListPtr,
    long groupId
    )
{
    GlGroup *groupPtr;

    for (groupPtr = *groupListPtr; groupPtr != NULL; groupPtr = groupPtr->next) {
        if (groupId == groupPtr->id) {
            return groupPtr->name;
        }
    }
    return "NoGroup";
}

/*++

GetOnlineData

    Retrieve online data from a glFTPD shared memory segment.

Arguments:
    interp        - Interpreter to use for error reporting.

    shmKey        - Shared memory key used by glFTPD.

    version       - Online structure version, must be an index in the 'versions' array.

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

    if (shmInfo.shm_segsz % versions[version].structSize) {
        Tcl_ResetResult(interp);
        Tcl_SetResult(interp, "unable to retrieve shared memory data: "
            "glftpd version mismatch", TCL_STATIC);
        return TCL_ERROR;
    }

    *maxUsers = shmInfo.shm_segsz / versions[version].structSize;

    if (!onlineDataPtr) {
        return TCL_OK;
    }

    // Copy data into the generic online structure.
    *onlineDataPtr = (GlOnlineGeneric **) ckalloc(sizeof(GlOnlineGeneric *) * (*maxUsers));

    switch (version) {
        case GLFTPD_130: {
            GlOnline130 *glData = (GlOnline130 *) shmData;

            for (i = 0; i < *maxUsers; i++) {
                entry = (GlOnlineGeneric *) ckalloc(sizeof(GlOnlineGeneric));

                memcpy(entry->tagline  ,  glData[i].tagline,    sizeof(glData[i].tagline));
                memcpy(entry->username,   glData[i].username,   sizeof(glData[i].username));
                memcpy(entry->status,     glData[i].status,     sizeof(glData[i].status));
                memcpy(entry->host,       glData[i].host,       sizeof(glData[i].host));
                memcpy(entry->currentdir, glData[i].currentdir, sizeof(glData[i].currentdir));
                entry->ssl_flag      = -1;      // Not present in glFTPD 1.3x.
                entry->groupid       = glData[i].groupid;
                entry->login_time    = glData[i].login_time;
                entry->tstart        = glData[i].tstart;
                entry->txfer         = {0, 0};  // Not present in glFTPD 1.3x.
                entry->bytes_xfer    = glData[i].bytes_xfer;
                entry->bytes_txfer   = 0;       // Not present in glFTPD 1.3x.
                entry->procid        = glData[i].procid;
                (*onlineDataPtr)[i]  = entry;
            }
        }
        case GLFTPD_200: {
            GlOnline200 *glData = (GlOnline200 *) shmData;

            for (i = 0; i < *maxUsers; i++) {
                entry = (GlOnlineGeneric *) ckalloc(sizeof(GlOnlineGeneric));

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
            // The 'GlOnlineGeneric' structure is the exact same
            // as the 'GlOnline201' structure (for now anyway).
            //
            assert(sizeof(GlOnlineGeneric) == sizeof(GlOnline201));

            for (i = 0; i < *maxUsers; i++) {
                entry = (GlOnlineGeneric *) ckalloc(sizeof(GlOnlineGeneric));
                memcpy(entry, shmData + (i * sizeof(GlOnline201)), sizeof(GlOnline201));
                (*onlineDataPtr)[i] = entry;
            }
        }
    }

    return TCL_OK;
}

/*++

FreeOnlineData

    Frees online data allocated by 'GetOnlineData'.

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
        ckfree((char *) onlineDataPtr[i]);
    }
    ckfree((char *) onlineDataPtr);
}

/*++

GlOpenCmd

    Creates a new glFTPD session handle.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a 'ExtState' structure.

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
    char handleId[20];
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

    handlePtr = (GlHandle *) ckalloc(sizeof(GlHandle));

    etcPath   = GLFTPD_ETC_PATH;
    etcLength = strlen(etcPath) + 1;

    handlePtr->etcPath = ckalloc(etcLength);
    strncpy(handlePtr->etcPath, etcPath, etcLength);
    handlePtr->etcPath[etcLength-1] = '\0';

    handlePtr->shmKey  = (key_t) shmKey;
    handlePtr->version = GLFTPD_201;

    // Create a hash table entry and return the handle's identifier.
    snprintf(handleId, ARRAYSIZE(handleId), "glftpd%lu", statePtr->glftpdCount);
    handleId[ARRAYSIZE(handleId)-1] = '\0';
    statePtr->glftpdCount++;

    hashEntryPtr = Tcl_CreateHashEntry(statePtr->glftpdTable, handleId, &newEntry);
    Tcl_SetHashValue(hashEntryPtr, (ClientData) handlePtr);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), handleId, -1);
    return TCL_OK;
}

/*++

GlConfigCmd

    Changes options for an existing glFTPD session handle.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a 'ExtState' structure.

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
    static const char *switches[] = {"-etc", "-key", "-version", NULL};
    enum switches {SWITCH_ETC, SWITCH_KEY, SWITCH_VERSION};

    if (objc < 3 || (!(objc & 1) && objc != 4)) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle ?switch? ?value? ?switch value?...");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->glftpdTable, "glftpd");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (GlHandle *) Tcl_GetHashValue(hashEntryPtr);

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
                        Tcl_NewLongObj((long) handlePtr->shmKey));
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
                Tcl_SetLongObj(Tcl_GetObjResult(interp), (long) handlePtr->shmKey);
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

    statePtr - Pointer to a 'ExtState' structure.

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
    handlePtr = (GlHandle *) Tcl_GetHashValue(hashEntryPtr);

    // Free the handle structure and remove the hash table entry.
    ckfree(handlePtr->etcPath);
    ckfree((char *) handlePtr);
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

        handlePtr = (GlHandle *) Tcl_GetHashValue(entryPtr);
        ckfree(handlePtr->etcPath);
        ckfree((char *) handlePtr);
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

    statePtr - Pointer to a 'ExtState' structure.

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
    static const char *options[] = {"handles", "maxusers", NULL};
    enum options {OPTION_HANDLES, OPTION_MAXUSERS};

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "option ?arg...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    resultPtr = Tcl_GetObjResult(interp);

    switch ((enum options) index) {
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
            handlePtr = (GlHandle *) Tcl_GetHashValue(hashEntryPtr);

            if (GetOnlineData(interp, handlePtr->shmKey, handlePtr->version, &maxUsers, NULL) != TCL_OK) {
                return TCL_ERROR;
            }

            Tcl_SetLongObj(resultPtr, (long) maxUsers);
            return TCL_OK;
        }
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}

/*++

GlKillCmd

    Kills the specified glFTPD process ID.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a 'ExtState' structure.

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
    handlePtr = (GlHandle *) Tcl_GetHashValue(hashEntryPtr);

    if (Tcl_GetLongFromObj(interp, objv[3], &procId) != TCL_OK) {
        return TCL_ERROR;
    }

    if (GetOnlineData(interp, handlePtr->shmKey, handlePtr->version, &maxUsers, &onlineData) != TCL_OK) {
        return TCL_ERROR;
    }

    for (i = 0; i < maxUsers; i++) {
        if (onlineData[i]->procid > 0 && onlineData[i]->procid == (pid_t) procId) {

            if (kill(onlineData[i]->procid, SIGTERM) == 0) {
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

    statePtr - Pointer to a 'ExtState' structure.

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
    GlGroup *groupListPtr = NULL;
    GlUser *userListPtr = NULL;
    Tcl_HashEntry *hashEntryPtr;
    Tcl_Obj **elementPtrs;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle fields");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->glftpdTable, "glftpd");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (GlHandle *) Tcl_GetHashValue(hashEntryPtr);

    if (Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementPtrs) != TCL_OK) {
        return TCL_ERROR;
    }

    // Never make assumptions on type sizes.
    fields = (unsigned char *) ckalloc(elementCount * sizeof(unsigned char));

    // Create an array of indices from 'whoFields'.
    for (i = 0; i < elementCount; i++) {
        if (Tcl_GetIndexFromObj(interp, elementPtrs[i], whoFields, "field", 0,
            &fieldIndex) != TCL_OK) {
            goto end;
        }

        if (fieldIndex == WHO_GROUP) {
            // Read '/glftpd/etc/group' for group ID to group name resolving.
            if (groupListPtr == NULL && GetGroupList(interp, handlePtr->etcPath,
                &groupListPtr) != TCL_OK) {
                goto end;
            }
        } else if (fieldIndex == WHO_UID) {
            // Read '/glftpd/etc/passwd' for user name to user ID resolving.
            if (userListPtr == NULL && GetUserList(interp, handlePtr->etcPath,
                &userListPtr) != TCL_OK) {
                goto end;
            }
        }

        fields[i] = (unsigned char) fieldIndex;
    }

    result = GetOnlineFields(interp, handlePtr, fields, elementCount,
        &userListPtr, &groupListPtr);

    end:
    if (userListPtr != NULL) {
        FreeUserList(&userListPtr);
    }
    if (groupListPtr != NULL) {
        FreeGroupList(&groupListPtr);
    }

    ckfree((char *) fields);
    return result;
}

/*++

GetOnlineFields

    Retrieve a list of online users and the requested fields.

Arguments:
    interp       - Current interpreter.

    handlePtr    - Pointer to a 'GlHandle' structure.

    fields       - Array of fields to retrieve.

    fieldCount   - Number of fields given for the 'fields' parameter.

    userListPtr  - Pointer to a 'GlUser' structure that represents the
                   user list head.

    groupListPtr - Pointer to a 'GlGroup' structure that represents the
                   group list head.

Return Value:
    A standard Tcl result.

Remarks:
    If the function succeeds, the user list is left in the interpreter's
    result. If the function fails, an error message is left instead.

--*/
static int
GetOnlineFields(
    Tcl_Interp *interp,
    GlHandle *handlePtr,
    unsigned char *fields,
    int fieldCount,
    GlUser **userListPtr,
    GlGroup **groupListPtr
    )
{
    int i;
    int j;
    int maxUsers;
    struct timeval timeNow;
    GlOnlineGeneric **onlineData;
    Tcl_Obj *elementObj;
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
        if (!onlineData[i]->procid) {
            continue;
        }
        userObj = Tcl_NewObj();

        for (j = 0; j < fieldCount; j++) {
            elementObj = NULL;

            switch ((int) fields[j]) {
                case WHO_ACTION: {
                    elementObj = Tcl_NewStringObj(onlineData[i]->status, -1);
                    break;
                }
                case WHO_GID: {
                    elementObj = Tcl_NewLongObj(onlineData[i]->groupid);
                    break;
                }
                case WHO_GROUP: {
                    elementObj = Tcl_NewStringObj(GetGroupNameFromId(groupListPtr,
                        onlineData[i]->groupid), -1);
                    break;
                }
                case WHO_HOST: {
                    elementObj = Tcl_NewStringObj(onlineData[i]->host, -1);
                    break;
                }
                case WHO_IDLETIME: {
                    elementObj = Tcl_NewLongObj((long) (timeNow.tv_sec - onlineData[i]->tstart.tv_sec));
                    break;
                }
                case WHO_LOGINTIME: {
                    elementObj = Tcl_NewLongObj((long) onlineData[i]->login_time);
                    break;
                }
                case WHO_PATH: {
                    elementObj = Tcl_NewStringObj(onlineData[i]->currentdir, -1);
                    break;
                }
                case WHO_PID: {
                    elementObj = Tcl_NewLongObj((long) onlineData[i]->procid);
                    break;
                }
                case WHO_SIZE: {
                    elementObj = Tcl_NewWideIntObj((Tcl_WideInt) onlineData[i]->bytes_xfer);
                    break;
                }
                case WHO_SPEED: {
                    double speed = (onlineData[i]->bytes_xfer / 1024.0) /
                        ((timeNow.tv_sec - onlineData[i]->tstart.tv_sec) * 1.0 +
                        (timeNow.tv_usec - onlineData[i]->tstart.tv_usec) / 1000000.0);

                    elementObj = Tcl_NewDoubleObj(speed);
                    break;
                }
                case WHO_SSL: {
                    elementObj = Tcl_NewLongObj((long) onlineData[i]->ssl_flag);
                    break;
                }
                case WHO_STATUS: {
                    long status = 0; // Idle

                    if (strncasecmp(onlineData[i]->status, "STOR ", 5) == 0 ||
                        strncasecmp(onlineData[i]->status, "APPE ", 5) == 0) {
                        status = 1; // Uploading
                    } else if (strncasecmp(onlineData[i]->status, "RETR ", 5) == 0) {
                        status = 2; // Downloading
                    }

                    elementObj = Tcl_NewLongObj(status);
                    break;
                }
                case WHO_TAGLINE: {
                    elementObj = Tcl_NewStringObj(onlineData[i]->tagline, -1);
                    break;
                }
                case WHO_UID: {
                    elementObj = Tcl_NewLongObj(GetUserIdFromName(userListPtr,
                        onlineData[i]->username));
                    break;
                }
                case WHO_USER: {
                    elementObj = Tcl_NewStringObj(onlineData[i]->username, -1);
                    break;
                }
            }

            assert(elementObj != NULL);
            Tcl_ListObjAppendElement(NULL, userObj, elementObj);
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
    clientData - Pointer to a 'ExtState' structure.

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
    ExtState *statePtr = (ExtState *) clientData;
    int index;
    static const char *options[] = {
        "close", "config", "info",
        "kill", "open", "who", NULL
    };
    enum options {
        OPTION_CLOSE, OPTION_CONFIG, OPTION_INFO,
        OPTION_KILL, OPTION_OPEN, OPTION_WHO
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_CLOSE:  return GlCloseCmd(interp, objc, objv, statePtr);
        case OPTION_CONFIG: return GlConfigCmd(interp, objc, objv, statePtr);
        case OPTION_INFO:   return GlInfoCmd(interp, objc, objv, statePtr);
        case OPTION_KILL:   return GlKillCmd(interp, objc, objv, statePtr);
        case OPTION_OPEN:   return GlOpenCmd(interp, objc, objv, statePtr);
        case OPTION_WHO:    return GlWhoCmd(interp, objc, objv, statePtr);
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
