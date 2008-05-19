/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2008 Alcoholicz Scripting Team

Module Name:
    Encoding

Author:
    neoxed (neoxed@gmail.com) May 21, 2005

Abstract:
    Encoding command definitions.

--*/

#ifndef _ALCOENCODING_H_
#define _ALCOENCODING_H_

typedef unsigned long (EncLengthProc)(
    unsigned long sourceLength
    );

typedef int (EncProcessProc)(
    const unsigned char *source,
    unsigned long sourceLength,
    unsigned char *dest,
    unsigned long *destLength
    );

typedef struct {
    char *name;
    EncLengthProc *GetDestLength;
    EncProcessProc *Process;
} EncodingFuncts;

extern const EncodingFuncts decodeFuncts[];
extern const EncodingFuncts encodeFuncts[];

Tcl_ObjCmdProc EncodingObjCmd;

#endif // _ALCOENCODING_H_
