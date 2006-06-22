/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Platform

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Portable platform definitions.

--*/

#ifndef _ALCOPLATFORM_H_
#define _ALCOPLATFORM_H_

#if defined(_WIN32) || defined(_WIN64) || defined(WINDOWS) || defined(_WINDOWS)
#   include "platwin.h"
#else
#   include "platunix.h"
#endif


//
// File I/O
//

FILE_HANDLE
FileOpen(
    const tchar_t *path,
    uint32_t access,
    uint32_t exists,
    uint32_t options
    );

bool_t
FileSize(
    FILE_HANDLE handle,
    uint64_t *size
    );

int64_t
FileSeek(
    FILE_HANDLE handle,
    int64_t offset,
    int method
    );

bool_t
FileRead(
    FILE_HANDLE handle,
    void *buffer,
    size_t bytesToRead,
    size_t *bytesRead
    );

bool_t
FileWrite(
    FILE_HANDLE handle,
    const void *buffer,
    size_t bytesToWrite,
    size_t *bytesWritten
    );

bool_t
FileClose(
    FILE_HANDLE handle
    );


//
// File Operations
//

bool_t
FileCopy(
    const tchar_t *sourcePath,
    const tchar_t *destPath,
    bool_t replace
    );

bool_t
FileMove(
    const tchar_t *sourcePath,
    const tchar_t *destPath,
    bool_t replace
    );

bool_t
FileRemove(
    const tchar_t *path
    );

bool_t
FileExists(
    const tchar_t *path
    );

bool_t
FileIsLink(
    const tchar_t *path
    );

bool_t
FileIsRegular(
    const tchar_t *path
    );


//
// Directory Operations
//

bool_t
DirCreate(
    const tchar_t *path
    );

bool_t
DirRemove(
    const tchar_t *path
    );

bool_t
DirExists(
    const tchar_t *path
    );


//
// Wide string macros
//

#undef TEXT
#undef _TEXT

#ifdef UNICODE
#   define _TEXT(s) L ## s
#else
#   define _TEXT(s) s
#endif

#define TEXT(s)     _TEXT(s)


//
// Portable tchar-like function mapping for standard C functions.
//

#ifdef UNICODE
// Main
#   define t_main           wmain

// I/O
#   define t_open           wopen
#   define t_fopen          wfopen
#   define t_fputc          fputwc

// Formatted I/O
#   define t_printf         wprintf
#   define t_fprintf        fwprintf
#   define t_sprintf        swprintf
#   define t_snprintf       snwprintf
#   define t_vprintf        vwprintf
#   define t_vfprintf       vfwprintf
#   define t_vsprintf       vswprintf
#   define t_vsnprintf      vsnwprintf

// Strings
#   define t_strcmp         wcscmp
#   define t_strncmp        wcsncmp
#   define t_strcasecmp     wcscasecmp
#   define t_strncasecmp    wcsncasecmp
#   define t_strchr         wcschr
#   define t_strdup         wcsdup
#   define t_strcspn        wcscspn
#   define t_strspn         wcsspn
#   define t_strpbrk        wcspbrk
#   define t_strstr         wcsstr
#   define t_strtok         wcstok
#   define t_strlen         wcslen

// String conversion
#   define t_strtod         wcstod
#   define t_strtol         wcstol
#   define t_strtoul        wcstoul
#else
// Main
#   define t_main           main

// I/O
#   define t_open           open
#   define t_fopen          fopen
#   define t_fputc          fputc

// Formatted I/O
#   define t_printf         printf
#   define t_fprintf        fprintf
#   define t_sprintf        sprintf
#   define t_snprintf       snprintf
#   define t_vprintf        vprintf
#   define t_vfprintf       vfprintf
#   define t_vsprintf       vsprintf
#   define t_vsnprintf      vsnprintf

// Strings
#   define t_strcmp         strcmp
#   define t_strncmp        strncmp
#   define t_strcasecmp     strcasecmp
#   define t_strncasecmp    strncasecmp
#   define t_strchr         strchr
#   define t_strdup         strdup
#   define t_strcspn        strcspn
#   define t_strspn         strspn
#   define t_strpbrk        strpbrk
#   define t_strstr         strstr
#   define t_strtok         strtok
#   define t_strlen         strlen

// String conversion
#   define t_strtod         strtod
#   define t_strtol         strtol
#   define t_strtoul        strtoul
#endif // UNICODE

#endif // _ALCOPLATFORM_H_
