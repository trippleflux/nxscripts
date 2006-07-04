/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Encoding

Author:
    neoxed (neoxed@gmail.com) Jun 30, 2006

Abstract:
    Encoding functions.

--*/

#include "alcoholicz.h"

static int currEncoding; // Current encoding, for file paths and output


/*++

EncInit

    Initialize the encoding subsystem and sets the locale.

Arguments:
    pool    - Main application pool, to create sub-pools from.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
EncInit(
    apr_pool_t *pool
    )
{
    apr_status_t status;
    int type;

    // Verify encoding names
    ASSERT(!strcmp(EncGetName(ENCODING_DEFAULT),  "DEFAULT"));
    ASSERT(!strcmp(EncGetName(ENCODING_ASCII),    "ASCII"));
    ASSERT(!strcmp(EncGetName(ENCODING_LATIN1),   "LATIN1"));
    ASSERT(!strcmp(EncGetName(ENCODING_UTF8),     "UTF-8"));
    ASSERT(!strcmp(EncGetName(ENCODING_UTF16_BE), "UTF-16BE"));
    ASSERT(!strcmp(EncGetName(ENCODING_UTF16_LE), "UTF-16LE"));
    ASSERT(!strcmp(EncGetName(ENCODING_UTF32_BE), "UTF-32BE"));
    ASSERT(!strcmp(EncGetName(ENCODING_UTF32_LE), "UTF-32LE"));

    // Retrieve the file path encoding used by APR
    status = apr_filepath_encoding(&type, pool);
    if (status != APR_SUCCESS) {
        return status;
    }

#ifdef WINDOWS
    // APR uses UTF-8 for Windows NT systems
    if (type != APR_FILEPATH_ENCODING_UTF8) {
        return APR_EINVAL;
    }
    currEncoding = ENCODING_UTF8;
#else
    // APR uses the locale for UNIX systems
    if (type != APR_FILEPATH_ENCODING_LOCALE) {
        return APR_EINVAL;
    }

    // Try to change the locale to UTF-8
    if (setlocale(LC_ALL, "UTF-8") != NULL) {
        currEncoding = ENCODING_UTF8;
    } else {
        setlocale(LC_ALL, "C");
        currEncoding = ENCODING_ASCII;
    }
#endif

    return APR_SUCCESS;
}

/*++

EncDetect

    Detects the encoding of a buffer.

Arguments:
    buffer  - Pointer to the buffer to examine.

    length  - Length of the buffer, in bytes.

    offset  - Pointer to a variable to recieve the offset of the data, in the
              case of a byte-order marker (zero if no BOM is present).

Return Values:
    Returns an encoding type identifier.

--*/
int
EncDetect(
    const apr_byte_t *buffer,
    apr_size_t length,
    apr_size_t *offset
    )
{
    int i;
    struct {
        char        *marker;  // Byte-order mark
        apr_size_t  length;   // Length of the BOM
        int         type;     // Encoding type identifer
    } static const bomTable[] = {
        {"\xFE\xFF",         2, ENCODING_UTF16_BE},
        {"\xFF\xFE",         2, ENCODING_UTF16_LE},
        {"\xEF\xBB\xBF",     3, ENCODING_UTF8},
        {"\x00\x00\xFE\xFF", 4, ENCODING_UTF32_BE},
        {"\xFF\xFE\x00\x00", 4, ENCODING_UTF32_LE}
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
            return bomTable[i].type;
        }
    }

    // No BOM = no offset
    *offset = 0;

    // UTF-8 encoded files are not always marked with a byte-order marker.

    // TODO:
    // - check for UTF-8 sequences in the buffer
    // - check for latin1 chars (>128)

    return ENCODING_ASCII;
}

/*++

EncGetCurrent

    Retrieves the current encoding type identifier.

Arguments:
    None.

Return Values:
    An encoding type identifier.

--*/
int
EncGetCurrent(
    void
    )
{
    return currEncoding;
}

/*++

EncGetName

    Retrieves the name of an encoding type identifier.

Arguments:
    type    - An encoding type identifier.

Return Values:
    Pointer to a null-terminated string that describes the encoding type identifier.

--*/
const
char *
EncGetName(
    int type
    )
{
    static const char *names[] = {
        "DEFAULT", "ASCII", "LATIN1", "UTF-8",
        "UTF-16BE", "UTF-16LE", "UTF-32BE", "UTF-32LE"
    };

    if (type >= 0 && type < ARRAYSIZE(names)) {
        return names[type];
    } else {
        return "UNKNOWN";
    }
}
