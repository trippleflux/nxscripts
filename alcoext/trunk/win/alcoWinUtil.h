/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoWinUtil.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 6, 2005
 *
 * Abstract:
 *   Miscellanenous Windows specific utilities.
 */

#ifndef __ALCOWINUTIL_H__
#define __ALCOWINUTIL_H__

/* Flags for IsFeatureAvailable(). */
#define FEATURE_DISKSPACEEX     0x00000001
#define FEATURE_MOUNT_POINTS    0x00000002

int IsFeatureAvailable(unsigned long features);
char *TclSetWinError(Tcl_Interp *interp, unsigned long errorCode);

#endif /* __ALCOWINUTIL_H__ */
