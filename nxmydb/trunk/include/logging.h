/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Logging

Author:
    neoxed (neoxed@gmail.com) Sep 15, 2007

Abstract:
    Logging function and macro declarations.

*/

#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED

//
// Debugger logging
//

DWORD FCALL LogDebugInit(VOID);
DWORD FCALL LogDebugFinalize(VOID);

VOID SCALL LogDebugFormat(const CHAR *format, ...);
VOID SCALL LogDebugFormatV(const CHAR *format, va_list argList);
VOID SCALL LogDebugTrace(const CHAR *file, const CHAR *func, INT line, const CHAR *format, ...);
VOID SCALL LogDebugTraceV(const CHAR *file, const CHAR *func, INT line, const CHAR *format, va_list argList);


//
// File logging
//

DWORD FCALL LogFileInit(VOID);
DWORD FCALL LogFileFinalize(VOID);

VOID SCALL LogFileFormat(const CHAR *format, ...);
VOID SCALL LogFileFormatV(const CHAR *format, va_list argList);
VOID SCALL LogFileTrace(const CHAR *file, const CHAR *func, INT line, const CHAR *format, ...);
VOID SCALL LogFileTraceV(const CHAR *file, const CHAR *func, INT line, const CHAR *format, va_list argList);


#ifdef DEBUG
#   define TRACE_HEAD()         LogDebugInit()
#   define TRACE_FOOT()         LogDebugFinalize()
#   define TRACE(format, ...)   LogDebugTrace(__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#else
#   define TRACE_HEAD()         ((VOID)0)
#   define TRACE_FOOT()         ((VOID)0)
#   define TRACE(format, ...)   ((VOID)0)
#endif


#endif // LOGGING_H_INCLUDED
