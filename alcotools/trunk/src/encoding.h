/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Encoding

Author:
    neoxed (neoxed@gmail.com) Jun 30, 2006

Abstract:
    Encoding function prototypes.

--*/

#ifndef _ENCODING_H_
#define _ENCODING_H_

typedef apr_uint32_t    utf32_t;
typedef apr_uint16_t    utf16_t;
typedef apr_byte_t      utf8_t;

typedef enum {
    ENCODING_ASCII = 0,
    ENCODING_LATIN1,
    ENCODING_UTF8,
    ENCODING_UTF16_BE,
    ENCODING_UTF16_LE,
    ENCODING_UTF32_BE,
    ENCODING_UTF32_LE
} ENCODING_TYPE;


apr_status_t
EncInit(
    apr_pool_t *pool
    );

ENCODING_TYPE
EncDetect(
    const apr_byte_t *buffer,
    apr_size_t length,
    apr_size_t *offset
    );

const
char *
EncGetName(
    ENCODING_TYPE type
    );

#endif // _ENCODING_H_
