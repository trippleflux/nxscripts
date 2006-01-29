/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    alcoEncoding.h

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

// Number of supported encoding types, including the NULL entry.
#define ENCODING_TYPES  2 + 1

const EncodingFuncts decodeFuncts[ENCODING_TYPES];
const EncodingFuncts encodeFuncts[ENCODING_TYPES];

Tcl_ObjCmdProc EncodingObjCmd;

#endif // _ALCOENCODING_H_
