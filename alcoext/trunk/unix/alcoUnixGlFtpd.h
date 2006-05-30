/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    glFTPD

Author:
    neoxed (neoxed@gmail.com) Apr 22, 2005

Abstract:
    glFTPD command definitions.

--*/

#ifndef _ALCOUNIXGLFTPD_H_
#define _ALCOUNIXGLFTPD_H_

// Force structure alignment to 4 bytes.
#pragma pack(push, 4)

// 32-bit timeval data structure.
typedef struct {
    int32_t tv_sec;
    int32_t tv_usec;
} timeval32_t;

// Version specific shared memory structures.
typedef struct {
    char        tagline[64];
    char        username[24];
    char        status[256];
    char        host[256];
    char        currentdir[256];
    int32_t     groupid;
    int32_t     login_time;
    timeval32_t tstart;
    uint32_t    bytes_xfer;
    int32_t     procid;
} GlOnline130;

typedef struct {
    char        tagline[64];
    char        username[24];
    char        status[256];
    int16_t     ssl_flag;
    char        host[256];
    char        currentdir[256];
    int32_t     groupid;
    int32_t     login_time;
    timeval32_t tstart;
    timeval32_t txfer;
    uint64_t    bytes_xfer;
    int32_t     procid;
} GlOnline200;

typedef struct {
    char        tagline[64];
    char        username[24];
    char        status[256];
    int16_t     ssl_flag;
    char        host[256];
    char        currentdir[256];
    int32_t     groupid;
    int32_t     login_time;
    timeval32_t tstart;
    timeval32_t txfer;
    uint64_t    bytes_xfer;
    uint64_t    bytes_txfer;
    int32_t     procid;
} GlOnline201;

//
// Generic shared memory structure. At the moment, this structure is a
// copy of the v2.01 online structure until the structure changes again.
//
typedef GlOnline201 GlOnlineGeneric;

// Restore default structure alignment for non-critical structures.
#pragma pack(pop)


// Default path to glFTPD's "etc" directory.
#define GLFTPD_ETC_PATH "/glftpd/etc"

// Name of the "group" and "passwd" files (must include a leading slash).
#define GLFTPD_GROUP    "/group"
#define GLFTPD_PASSWD   "/passwd"

typedef struct GlNames GlNames;
struct GlNames {
    int32_t  id;       // User/group ID.
    char     name[24]; // User/group name.
    GlNames  *next;    // Pointer to the next user/group.
};

typedef struct {
    char *name;
    size_t structSize;
} GlVersion;

typedef struct {
    char *etcPath; // Path to glFTPD's "etc" directory.
    key_t shmKey;  // Shared memory segment key.
    int version;   // Array index in "versions", representing the online structure version.
} GlHandle;

Tcl_ObjCmdProc GlFtpdObjCmd;

void
GlCloseHandles(
    Tcl_HashTable *tablePtr
    );

#endif // _ALCOUNIXGLFTPD_H_
