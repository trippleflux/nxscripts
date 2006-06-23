/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    CRC-32

Author:
    neoxed (neoxed@gmail.com) Nov 3, 2005

Abstract:
    CRC-32 checksum function prototypes.

--*/

#ifndef _CRC32_H_
#define _CRC32_H_

apr_uint32_t
Crc32File(
    apr_file_t *file
    );

apr_uint32_t
Crc32Memory(
    const void *memory,
    apr_uint32_t size
    );

apr_uint32_t
Crc32String(
    const char *string
    );

apr_uint32_t
Crc32UpperString(
    const char *string
    );

#endif // _CRC32_H_
