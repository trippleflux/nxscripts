/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements miscellaneous utilities.

--*/

#include "alcoholicz.h"

/*++

BufferFile

    Reads the contents of a file into a buffer.

Arguments:
    path    - Pointer to a null-terminated string that specifies the file path.

    buffer  - Pointer to a pointer to the file's contents.

    length  - Pointer to a variable to store the buffer's length, in bytes.

    pool    - Pool to allocate buffer from.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
BufferFile(
    const char *path,
    apr_byte_t **buffer,
    apr_size_t *length,
    apr_pool_t *pool
    )
{
    apr_byte_t *data;
    apr_file_t *file;
    apr_finfo_t info;
    apr_size_t amount;
    apr_status_t status;

    ASSERT(path != NULL);
    ASSERT(buffer != NULL);
    ASSERT(length != NULL);
    ASSERT(pool != NULL);

    LOG_DEBUG("Buffering file \"%s\" into memory pool.", path);

    // Open configuration file for reading
    status = apr_file_open(&file, path, APR_FOPEN_READ, APR_OS_DEFAULT, pool);
    if (status != APR_SUCCESS) {
        return status;
    }

    // Retrieve file size
    status = apr_file_info_get(&info, APR_FINFO_SIZE, file);
    if (status == APR_SUCCESS) {

        // Allocate a buffer large enough to contain the file
        amount = (apr_size_t)info.size;
        data = apr_palloc(pool, amount);
        if (data == NULL) {
            status = APR_ENOMEM;
        } else {
            // Read file into buffer
            status = apr_file_read_full(file, data, amount, &amount);
            if (status == APR_SUCCESS) {
                *buffer = data;
                *length = amount;
            }
        }
    }

    apr_file_close(file);
    return status;
}

/*++

GetErrorMessage

    Returns a string explaining the APR status code.

Arguments:
    status  - APR status code.

Return Values:
    Pointer to a statically allocated buffer containing a human-readable
    message explaining the APR status code. The contents of this buffer
    must not be modified.

Remarks:
    This function is not thread-safe.

--*/
const char *
GetErrorMessage(
    apr_status_t status
    )
{
    static char message[512]; // THREADING: Static variable not thread-safe.
    return apr_strerror(status, message, ARRAYSIZE(message));
}

#ifdef WINDOWS
/*++

ReadConsoleFullW

    Wrapper around ReadConsoleW() that ensures the buffer is filled.

Arguments:
    console     - Handle to the console input buffer.

    buffer      - Pointer to a buffer that receives the data.

    charsToRead - Number of characters to read.

    charsRead   - Pointer to a variable that receives the number of characters read.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
ReadConsoleFullW(
    HANDLE console,
    void *buffer,
    DWORD charsToRead,
    DWORD *charsRead
    )
{
    DWORD amountRead;
    DWORD totalRead = 0;
    BOOL result;

    do {
        result = ReadConsoleW(console, buffer, charsToRead, &amountRead, NULL);
        charsToRead -= amountRead;

        buffer = ((WCHAR *)buffer) + amountRead;
        totalRead += amountRead;
    } while (result && charsToRead > 0);

    *charsRead = totalRead;
    return result;
}

/*++

WriteConsoleFullW

    Wrapper around WriteConsoleW() that ensures all of the data was written.

Arguments:
    console      - Handle to the console output buffer.

    buffer       - Pointer to a buffer that contains the data to be written.

    charsToWrite - Number of characters to write.

    charsWritten - Pointer to a variable that receives the number of characters written.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
WriteConsoleFullW(
    HANDLE console,
    const void *buffer,
    DWORD charsToWrite,
    DWORD *charsWritten
    )
{
    DWORD amountWritten;
    DWORD totalWritten = 0;
    BOOL result;

    do {
        result = WriteConsoleW(console, buffer, charsToWrite, &amountWritten, NULL);
        charsToWrite -= amountWritten;

        buffer = ((WCHAR *)buffer) + amountWritten;
        totalWritten += amountWritten;
    } while (result && charsToWrite > 0);

    *charsWritten = totalWritten;
    return result;
}
#endif // WINDOWS
