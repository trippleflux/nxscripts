/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    alcoCrypt.h

Author:
    neoxed (neoxed@gmail.com) May 6, 2005

Abstract:
    Cryptographic command definitions.

--*/

#ifndef _ALCOCRYPT_H_
#define _ALCOCRYPT_H_

void
CryptCloseHandles(
    Tcl_HashTable *tablePtr
    );

Tcl_ObjCmdProc CryptObjCmd;

#endif // _ALCOCRYPT_H_
