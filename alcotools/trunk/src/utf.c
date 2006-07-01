/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    UTF

Author:
    neoxed (neoxed@gmail.com) Jun 30, 2006

Abstract:
    UTF encoding functions.

--*/

#include "alcoholicz.h"

/*++

UtfDetectEncoding

    Detects the encoding type of the specified buffer.

Arguments:
    buffer  - Pointer to the buffer to examine.

    length  - Length of the buffer, in bytes.

    offset  - Pointer to a variable to recieve the offset of the data, in the
              case of a byte order marker (zero if no BOM is present).

Return Values:
    Returns an ENCODING_TYPE identifier.

--*/
ENCODING_TYPE
UtfDetectEncoding(
    const apr_byte_t *buffer,
    apr_size_t length,
    apr_size_t *offset
    )
{
    int i;
    struct {
        char *marker;
        apr_size_t length;
    } static const bomTable[] = {
        {"\xFE\xFF",         2},    // UTF-16, big endian
        {"\xFF\xFE",         2},    // UTF-16, little endian
        {"\xEF\xBB\xBF",     3},    // UTF-8
        {"\x00\x00\xFE\xFF", 4},    // UTF-32, big endian
        {"\xFF\xFE\x00\x00", 4}     // UTF-32, little endian
    };

    ASSERT(buffer != NULL);
    ASSERT(offset != NULL);

    for (i = 0; i < ARRAYSIZE(bomTable); i++) {
        // The BOMs are arranged in increasing order according to their length,
        // so we can exit the loop if the buffer's length is less than the BOM's.
        if (length < bomTable[i].length) {
            break;
        }

        if (!memcmp(buffer, bomTable[i].marker, bomTable[i].length)) {
            *offset = bomTable[i].length;
            return i;
        }
    }

    // No BOM = no offset
    *offset = 0;

    // UTF-8 encoded files are not always marked with a byte order marker.
    // TODO: check for UTF-8 sequences in the buffer

    return ENCODING_NONE;
}
