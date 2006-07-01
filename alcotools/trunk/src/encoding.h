/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    UTF

Author:
    neoxed (neoxed@gmail.com) Jun 30, 2006

Abstract:
    UTF encoding function prototypes.

--*/

#ifndef _UTF_H_
#define _UTF_H_

typedef enum {
    ENCODING_UTF16_BE = 0,
    ENCODING_UTF16_LE,
    ENCODING_UTF8,
    ENCODING_UTF32_BE,
    ENCODING_UTF32_LE,
    ENCODING_NONE
} ENCODING_TYPE;

ENCODING_TYPE
UtfDetectEncoding(
    const apr_byte_t *buffer,
    apr_size_t length,
    apr_size_t *offset
    );

#endif // _UTF_H_
