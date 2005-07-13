/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoUnixGlFtpd.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 22, 2005
 *
 * Abstract:
 *   glFTPD specific definitions.
 */

#ifndef _ALCOUNIXGLFTPD_H_
#define _ALCOUNIXGLFTPD_H_

/* Default path to glFTPD's 'etc' directory. */
#define GLFTPD_ETC_PATH "/glftpd/etc"

/* Name of the 'group' and 'passwd' files (must include a leading slash). */
#define GLFTPD_GROUP    "/group"
#define GLFTPD_PASSWD   "/passwd"

/*
 * Generic shared memory structure. At the moment, this structure is a
 * copy of the v2.01 online structure until the structure changes again.
 */
typedef struct {
    char   tagline[64];
    char   username[24];
    char   status[256];
    short int ssl_flag;
    char   host[256];
    char   currentdir[256];
    long   groupid;
    time_t login_time;
    struct timeval tstart;
    struct timeval txfer;
    unsigned long long bytes_xfer;
    unsigned long long bytes_txfer;
    pid_t  procid;
} GlOnlineGeneric;

/* Version specific shared memory structures. */
typedef struct {
    char   tagline[64];
    char   username[24];
    char   status[256];
    char   host[256];
    char   currentdir[256];
    long   groupid;
    time_t login_time;
    struct timeval tstart;
    unsigned long bytes_xfer;
    pid_t  procid;
} GlOnline130;

typedef struct {
    char   tagline[64];
    char   username[24];
    char   status[256];
    short int ssl_flag;
    char   host[256];
    char   currentdir[256];
    long   groupid;
    time_t login_time;
    struct timeval tstart;
    struct timeval txfer;
    unsigned long long bytes_xfer;
    pid_t  procid;
} GlOnline200;

typedef struct {
    char   tagline[64];
    char   username[24];
    char   status[256];
    short int ssl_flag;
    char   host[256];
    char   currentdir[256];
    long   groupid;
    time_t login_time;
    struct timeval tstart;
    struct timeval txfer;
    unsigned long long bytes_xfer;
    unsigned long long bytes_txfer;
    pid_t  procid;
} GlOnline201;

#define GL_USER_LENGTH      24
#define GL_GROUP_LENGTH     24

typedef struct GlGroup {
    long id;
    char name[GL_GROUP_LENGTH];
    struct GlGroup *next;
} GlGroup;

typedef struct GlUser {
    long id;
    char name[GL_USER_LENGTH];
    struct GlUser *next;
} GlUser;

typedef struct {
    char *name;
    unsigned long structSize;
} GlVersion;

typedef struct {
    char *etcPath;
    key_t shmKey;
    int version;
} GlHandle;

Tcl_ObjCmdProc GlFtpdObjCmd;
void GlCloseHandles(Tcl_HashTable *tablePtr);

#endif /* _ALCOUNIXGLFTPD_H_ */
