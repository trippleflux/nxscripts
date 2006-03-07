/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxMacro.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Miscellaneous macros.
 */

#ifndef _NXMACROS_H_
#define _NXMACROS_H_

#ifdef ARRAYSIZE
#   undef ARRAYSIZE
#endif
#ifdef ROUNDUP
#   undef ROUNDUP
#endif

/* ARRAYSIZE - Returns the number of entries in an array. */
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

/* ROUNDUP - Round 'a' up to a multiple of 'b'. */
#define ROUNDUP(a,b)    ((((a) + ((b) - 1)) / (b)) * (b))


#if defined(UNICODE) && (TCL_UTF_MAX != 3)
#error "Unsupported TCL_UTF_MAX value, must be 3."
#endif

/* TCHAR's for Tcl string functions. */
#ifdef UNICODE
#define Tcl_GetTString Tcl_GetUnicode
#define Tcl_GetTStringFromObj Tcl_GetUnicodeFromObj
#define Tcl_NewTStringObj Tcl_NewUnicodeObj
#define Tcl_SetTStringObj Tcl_SetUnicodeObj
#else
#define Tcl_GetTString Tcl_GetString
#define Tcl_GetTStringFromObj Tcl_GetStringFromObj
#define Tcl_NewTStringObj Tcl_NewStringObj
#define Tcl_SetTStringObj Tcl_SetStringObj
#endif

#endif /* _MACROS_H_ */
