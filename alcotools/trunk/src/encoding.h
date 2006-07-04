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

// UTF data types
typedef apr_uint32_t    utf32_t;
typedef apr_uint16_t    utf16_t;
typedef apr_byte_t      utf8_t;

// Enocding identifiers
#define ENCODING_UNKNOWN    -1
#define ENCODING_ASCII      0
#define ENCODING_LATIN1     1
#define ENCODING_UTF8       2
#define ENCODING_UTF16_BE   3
#define ENCODING_UTF16_LE   4
#define ENCODING_UTF32_BE   5
#define ENCODING_UTF32_LE   6


apr_status_t
EncInit(
    apr_pool_t *pool
    );

int
EncDetect(
    const apr_byte_t *buffer,
    apr_size_t length,
    apr_size_t *offset
    );

const
char *
EncGetName(
    int encoding
    );

#endif // _ENCODING_H_
