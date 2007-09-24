/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous debugging utilities.

*/

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

//
// Memory debugging
//

#ifdef DEBUG
VOID *SCALL DebugAllocate(SIZE_T Bytes, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine);
VOID *SCALL DebugReallocate(VOID *Mem, SIZE_T Bytes, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine);
BOOL  SCALL DebugFree(VOID *Mem, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine);
VOID  SCALL DebugCheck(VOID *Mem, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine);
#endif // DEBUG

#ifdef DEBUG
    #define MemAllocate(bytes)          DebugAllocate((bytes), __FILE__, __FUNCTION__, __LINE__)
    #define MemReallocate(mem, bytes)   DebugReallocate((mem), (bytes), __FILE__, __FUNCTION__, __LINE__)
    #define MemFree(mem)                DebugFree((mem), __FILE__, __FUNCTION__, __LINE__)
    #define MemCheck(mem)               DebugCheck((mem), __FILE__, __FUNCTION__, __LINE__)
#else
    #define MemAllocate(bytes)          Io_Allocate((bytes))
    #define MemReallocate(mem, bytes)   Io_ReAllocate((mem), (bytes))
    #define MemFree(mem)                Io_Free((mem))
    #define MemCheck(mem)               ((VOID)0)
#endif // DEBUG


//
// Message tracing
//

#ifdef DEBUG
VOID FCALL TraceHeader(VOID);
VOID CCALL TraceFormat(const char *funct, const char *format, ...);
VOID FCALL TraceFooter(VOID);
#endif // DEBUG

#endif // DEBUG_H_INCLUDED
