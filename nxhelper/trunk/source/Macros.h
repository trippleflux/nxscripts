#ifndef __MACROS_H__
#define __MACROS_H__

#if defined(UNICODE) && (TCL_UTF_MAX != 3)
#error "Unsupported TCL_UTF_MAX value, must be 3."
#endif

// TCHAR's for Tcl string functions
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

#endif // __MACROS_H__