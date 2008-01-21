/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2008 Alcoholicz Scripting Team

Module Name:
    Unix Utilities

Author:
    neoxed (neoxed@gmail.com) Mar 14, 2006

Abstract:
    Unix utility function declarations.

--*/

#ifndef _TCLDUNIXUTIL_H_
#define _TCLDUNIXUTIL_H_

//
// Determine if the given file exists.
//
#define FileExists(path) \
    (access(path, F_OK) == 0)

//
// Writes a timestamp to the given file.
//
#define WriteTime(handle)                                 \
    do {                                                  \
        time_t timer;                                     \
        struct tm *now;                                   \
        time(&timer);                                     \
        now = localtime(&timer);                          \
                                                          \
        fprintf(handle, "%04d-%02d-%02d %02d:%02d:%02d ", \
            now->tm_year+1900, now->tm_mon, now->tm_mday, \
            now->tm_hour, now->tm_min, now->tm_sec);      \
    } while (0);

#endif // _TCLDUNIXUTIL_H_
