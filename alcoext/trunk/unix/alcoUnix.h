/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoUnix.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 16, 2005
 *
 * Abstract:
 *   Unix specific includes, macros, and function declarations.
 */

#ifndef __ALCOUNIX_H__
#define __ALCOUNIX_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif /* HAVE_INTTYPES_H */

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */

#ifdef HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif /* HAVE_SYS_MOUNT_H */

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif /* HAVE_SYS_STATFS_H */

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif /* HAVE_SYS_STATVFS_H */

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif /* HAVE_SYS_VFS_H */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#include "alcoUnixGlFtpd.h"

#endif /* __ALCOUNIX_H__ */
