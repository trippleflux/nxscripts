/*
 * AlcoTcld - Alcoholicz Tcl daemon.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   tcldUnix.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) July 17, 2005
 *
 * Abstract:
 *   BSD/Linux/UNIX specific includes and defintions.
 */

#ifndef _TCLDUNIX_H_
#define _TCLDUNIX_H_

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_INTTYPES_H
#   include <inttypes.h>
#endif
#ifdef HAVE_LIMITS_H
#   include <limits.h>
#endif
#ifdef HAVE_STDINT_H
#   include <stdint.h>
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
#ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#endif

#endif /* _TCLDUNIX_H_ */
