/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoUnix.h

Author:
    neoxed (neoxed@gmail.com) April 16, 2005

Abstract:
    BSD/Linux/UNIX specific headers and macros.

--*/

#ifndef _ALCOUNIX_H_
#define _ALCOUNIX_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_STRINGS_H
#   include <strings.h>
#endif
#ifdef HAVE_TIME_H
#   include <time.h>
#endif
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#   include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#   include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#   include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#   include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#endif

#ifdef HAVE_LIMITS_H
#   include <limits.h>
#endif
#ifdef HAVE_SYSLIMITS_H
#   include <syslimits.h>
#endif
#ifdef HAVE_SYS_LIMITS_H
#   include <sys/limits.h>
#endif

#include "alcoUnixGlFtpd.h"

// Statfs function and structure checks.
#ifdef STAT_STATVFS64
#   define STATFS_FN(path, buf) (statvfs64(path,buf))
#   define STATFS_T statvfs64
#   define USE_STATVFS
#elif defined(STAT_STATVFS)
#   define STATFS_FN(path, buf) (statvfs(path,buf))
#   define STATFS_T statvfs
#   define USE_STATVFS
#elif defined(STAT_STATFS4)
// For implementations of statfs() that take four parameters.
#   define STATFS_FN(path, buf) (statfs(path,buf,sizeof(buf),0))
#   define STATFS_T statfs
#   define USE_STATFS
#elif defined(STAT_STATFS3_OSF1)
// For implementations of statfs() that take three parameters.
#   define STATFS_FN(path, buf) (statfs(path,buf,sizeof(buf)))
#   define STATFS_T statfs
#   define USE_STATFS
#elif defined(STAT_STATFS2_FS_DATA) || defined(STAT_STATFS2_BSIZE) || defined(STAT_STATFS2_FSIZE)
// For implementations of statfs() that take two parameters.
#   define STATFS_FN(path, buf) (statfs(path,buf))
#   define STATFS_T statfs
#   define USE_STATFS
#endif // STAT_STATVFS64

// Check for the f_basetype or f_fstypename member.
#if (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_BASETYPE)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_BASETYPE))
#   define F_TYPENAME(buf) ((buf).f_basetype)
#elif (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_FSTYPENAME)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_FSTYPENAME))
#   define F_TYPENAME(buf) ((buf).f_fstypename)
#else
#   define F_TYPENAME(buf) ("")
#endif

// Check for the f_fsid.val or f_fsid member.
#if (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_FSID_VAL)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_FSID_VAL))
//
// The contents of the f_fsid member on BSD systems is as follows.
// f_fsid::val[0] - The dev_t identifier for the device, which is
//                  all that we're interested in.
// f_fsid::val[1] - Type of file system, MOUNT_xxx flag.
//
#   define F_FSID(buf) ((buf).f_fsid.val[0])
#elif (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_FSID)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_FSID))
#   define F_FSID(buf) ((buf).f_fsid)
#else
#   define F_FSID(buf) (0)
#endif

// Check for the f_flag or f_flags member.
#if (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_FLAG)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_FLAG))
#   define F_FLAGS(buf) ((buf).f_flag)
#elif (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_FLAGS)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_FLAGS))
#   define F_FLAGS(buf) ((buf).f_flags)
#else
#   define F_FLAGS(buf) (0)
#endif

// Check for the f_namemax or f_namelen member.
#if (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_NAMEMAX)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_NAMEMAX))
#   define F_NAMELEN(buf) ((buf).f_namemax)
#elif (defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_NAMELEN)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_NAMELEN))
#   define F_NAMELEN(buf) ((buf).f_namelen)
#else
#   define F_NAMELEN(buf) (255)
#endif

#endif // _ALCOUNIX_H_
