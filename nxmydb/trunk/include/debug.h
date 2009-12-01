/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

Module Name:
    Debugging

Author:
    neoxed (neoxed@gmail.com) Aug 3, 2009

Abstract:
    Debugging function and macro declarations.

*/

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifdef DEBUG

BOOL FCALL IsCriticalSectionOwned(CRITICAL_SECTION *critSection);
BOOL FCALL IsCriticalSectionCurrentOwner(CRITICAL_SECTION *critSection);
void FCALL WaitForDebugger(VOID);

#define ASSERT_CS_IS_OWNED(x)         (ASSERT(IsCriticalSectionOwned(x)))
#define ASSERT_CS_IS_CURRENT_OWNER(x) (ASSERT(IsCriticalSectionCurrentOwner(x)))

#else // DEBUG

#define IsCriticalSectionOwned(x)        ((VOID)0)
#define IsCriticalSectionCurrentOwner(x) ((VOID)0)
#define WaitForDebugger()                ((VOID)0)

#define ASSERT_CS_IS_OWNED(x)            ((VOID)0)
#define ASSERT_CS_IS_CURRENT_OWNER(x)    ((VOID)0)

#endif // DEBUG

#endif // DEBUG_H_INCLUDED
