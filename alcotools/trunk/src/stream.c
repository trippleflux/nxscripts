/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Stream

Author:
    neoxed (neoxed@gmail.com) Jul 5, 2006

Abstract:
    I/O stream functions.

--*/

#include "alcoholicz.h"

struct STREAM {
    void                *opaque;    // Opaque argument, managed by the caller
    STREAM_READ_PROC    *readProc;  // Procedure called to read data (produce)
    STREAM_WRITE_PROC   *writeProc; // Procedure called to write data (consume)
    STREAM_FLUSH_PROC   *flushProc; // Procedure called to flush the stream
    STREAM_CLOSE_PROC   *closeProc; // Procedure called when the stream is closed
};

/*++

StreamCreate

    Creates a new I/O stream.

Arguments:
    opaque      - Opaque argument passed to the callbacks. This argument can be
                  null if not required.

    readProc    - Procedure called to read from the stream. This argument can be
                  null if not required.

    writeProc   - Procedure called to write to the stream. This argument can be
                  null if not required.

    flushProc   - Procedure called to flush the stream. This argument can be
                  null if not required.

    closeProc   - Procedure called when the stream is closed. This argument can
                  be null if not required.

    pool        - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a STREAM structure.

    If the function fails, the return value is null.

--*/
STREAM *
StreamCreate(
    void *opaque,
    STREAM_READ_PROC *readProc,
    STREAM_WRITE_PROC *writeProc,
    STREAM_FLUSH_PROC *flushProc,
    STREAM_CLOSE_PROC *closeProc,
    apr_pool_t *pool
    )
{
    STREAM *stream;

    ASSERT(pool != NULL);

    // Populate the stream structure
    stream = apr_palloc(pool, sizeof(STREAM));
    if (stream != NULL) {
        stream->opaque     = opaque;
        stream->readProc   = readProc;
        stream->writeProc  = writeProc;
        stream->flushProc  = flushProc;
        stream->closeProc  = closeProc;
    }

    return stream;
}

/*++

StreamRead

    Reads data from an I/O stream.

Arguments:
    stream      - Pointer to a stream.

    buffer      - Pointer to the buffer that receives the data.

    bytesToRead - Number of bytes to read.

    bytesRead   - Pointer to a variable that receives the number of bytes read.
                  This argument can be null if not required.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
StreamRead(
    STREAM *stream,
    apr_byte_t *buffer,
    apr_size_t bytesToRead,
    apr_size_t *bytesRead
    )
{
    ASSERT(stream != NULL);
    ASSERT(buffer != NULL);

    if (stream->readProc == NULL) {
        return APR_ENOTIMPL;
    }
    return stream->readProc(stream->opaque, buffer, bytesToRead, bytesRead);
}

/*++

StreamWrite

    Writes data to an I/O stream.

Arguments:
    stream       - Pointer to a stream.

    buffer       - Pointer to the buffer containing the data to be written.

    bytesToWrite - Number of bytes to write.

    bytesWritten - Pointer to a variable that receives the number of bytes
                   written. This argument can be null if not required.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
StreamWrite(
    STREAM *stream,
    const apr_byte_t *buffer,
    apr_size_t bytesToWrite,
    apr_size_t *bytesWritten
    )
{
    ASSERT(stream != NULL);
    ASSERT(buffer != NULL);

    if (stream->writeProc == NULL) {
        return APR_ENOTIMPL;
    }
    return stream->writeProc(stream->opaque, buffer, bytesToWrite, bytesWritten);
}

/*++

StreamPuts

    Writes a string to an I/O stream.

Arguments:
    stream  - Pointer to a stream.

    str     - Pointer to a null-terminated string.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
StreamPuts(
    STREAM *stream,
    const char *str
    )
{
    ASSERT(stream != NULL);
    ASSERT(str != NULL);

    return StreamWrite(stream, (const apr_byte_t *)str, strlen(str), NULL);
}

/*++

StreamFlush

    Flushes an I/O stream.

Arguments:
    stream  - Pointer to a stream.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
StreamFlush(
    STREAM *stream
    )
{
    ASSERT(stream != NULL);

    if (stream->flushProc == NULL) {
        return APR_ENOTIMPL;
    }
    return stream->flushProc(stream->opaque);
}

/*++

StreamClose

    Closes an I/O stream.

Arguments:
    stream  - Pointer to a stream.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
StreamClose(
    STREAM *stream
    )
{
    ASSERT(stream != NULL);

    if (stream->closeProc == NULL) {
        return APR_SUCCESS;
    }
    return stream->closeProc(stream->opaque);
}


/*++

StreamCreateBinaryConsole

    Creates a binary console I/O stream.

Arguments:
    console - Console type.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a STREAM structure.

    If the function fails, the return value is null.

--*/
STREAM *
StreamCreateBinaryConsole(
    int console,
    apr_pool_t *pool
    )
{
    apr_file_t *file;
    apr_status_t status;

    ASSERT(pool != NULL);

    switch (console) {
        case CONSOLE_INPUT:
            status = apr_file_open_stdin(&file, pool);
            break;
        case CONSOLE_OUTPUT:
            status = apr_file_open_stdout(&file, pool);
            break;
        case CONSOLE_ERROR:
            status = apr_file_open_stderr(&file, pool);
            break;
        default:
            return NULL;
    }

    if (status != APR_SUCCESS) {
        return NULL;
    }

    return StreamCreateFile(file, TRUE, pool);
}


#ifdef WINDOWS
static
apr_status_t
TextConsoleRead(
    void *opaque,
    apr_byte_t *buffer,
    apr_size_t bytesToRead,
    apr_size_t *bytesRead
    )
{
    apr_status_t status;
    apr_size_t bytesRemaining = bytesToRead;
    apr_size_t charsRemaining;
    apr_wchar_t inBuffer[1];
    char *offset;
    HANDLE console = opaque;

    ASSERT(opaque != NULL);
    ASSERT(buffer != NULL);
    ASSERT(console != INVALID_HANDLE_VALUE);
    ASSERT(EncGetCurrent() == ENCODING_UTF8);

    do {
        // Read one character at a time so we don't over-request
        if (!ReadConsoleFullW(console, inBuffer, 1, (DWORD *)&charsRemaining)) {
            status = apr_get_os_error();
        } else {
            // Convert the UCS-2 character to UTF-8
            offset = (char *)buffer + (bytesToRead - bytesRemaining);
            status = apr_conv_ucs2_to_utf8(inBuffer, &charsRemaining, offset, &bytesRemaining);
        }
    } while (status == APR_SUCCESS && bytesRemaining > 0);

    if (bytesRead != NULL) {
        *bytesRead = bytesToRead - bytesRemaining;
    }
    return status;
}

static
apr_status_t
TextConsoleWrite(
    void *opaque,
    const apr_byte_t *buffer,
    apr_size_t bytesToWrite,
    apr_size_t *bytesWritten
    )
{
    apr_status_t status;
    apr_size_t bytesRemaining = bytesToWrite;
    apr_size_t charsRemaining;
    apr_wchar_t outBuffer[256];
    const char *offset;
    DWORD charsToWrite;
    DWORD charsWritten;
    HANDLE console = opaque;

    ASSERT(opaque != NULL);
    ASSERT(buffer != NULL);
    ASSERT(console != INVALID_HANDLE_VALUE);
    ASSERT(EncGetCurrent() == ENCODING_UTF8);

    do {
        // Convert the UTF-8 buffer to UCS-2
        charsRemaining = ARRAYSIZE(outBuffer);
        offset = (const char *)buffer + (bytesToWrite - bytesRemaining);
        status = apr_conv_utf8_to_ucs2(offset, &bytesRemaining, outBuffer, &charsRemaining);
        if (status == APR_EINVAL) {
            break;
        }

        // Write the UCS-2 buffer to the console
        charsToWrite = (DWORD)(ARRAYSIZE(outBuffer) - charsRemaining);
        if (WriteConsoleFullW(console, outBuffer, charsToWrite, &charsWritten)) {
            status = APR_SUCCESS;
        } else {
            status = apr_get_os_error();
        }
    } while (status == APR_SUCCESS && bytesRemaining > 0);

    if (bytesWritten != NULL) {
        *bytesWritten = bytesToWrite - bytesRemaining;
    }
    return status;
}

static
apr_status_t
TextConsoleFlush(
    void *opaque
    )
{
    // There's no need to flush the console, this function is only
    // implemented to remain compatible with *nix text console streams.
    return APR_SUCCESS;
}
#endif // WINDOWS

/*++

StreamCreateTextConsole

    Creates a text console I/O stream.

Arguments:
    console - Console type.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a STREAM structure.

    If the function fails, the return value is null.

--*/
STREAM *
StreamCreateTextConsole(
    int console,
    apr_pool_t *pool
    )
{
#ifdef WINDOWS
    DWORD type;
    HANDLE handle;

    ASSERT(pool != NULL);

    switch (console) {
        case CONSOLE_INPUT:
            type = STD_INPUT_HANDLE;
            break;
        case CONSOLE_OUTPUT:
            type = STD_OUTPUT_HANDLE;
            break;
        case CONSOLE_ERROR:
            type = STD_ERROR_HANDLE;
            break;
        default:
            return NULL;
    }

    // Retrieve standard device handle
    handle = GetStdHandle(type);
    if (handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    // There is no close callback because closing the handle returned
    // by GetStdHandle() would close the standard device.
    return StreamCreate(handle, TextConsoleRead,
        TextConsoleWrite, TextConsoleFlush, NULL, pool);
#else
    // TODO: See if there's anything special when writing
    // UTF-8 text to stdout/stderr on *nix systems.
    return StreamCreateBinaryConsole(console, pool);
#endif // WINDOWS
}


static
apr_status_t
FileRead(
    void *opaque,
    apr_byte_t *buffer,
    apr_size_t bytesToRead,
    apr_size_t *bytesRead
    )
{
    apr_file_t *file = opaque;

    ASSERT(opaque != NULL);
    ASSERT(buffer != NULL);

    return apr_file_read_full(file, buffer, bytesToRead, bytesRead);
}

static
apr_status_t
FileWrite(
    void *opaque,
    const apr_byte_t *buffer,
    apr_size_t bytesToWrite,
    apr_size_t *bytesWritten
    )
{
    apr_file_t *file = opaque;

    ASSERT(opaque != NULL);
    ASSERT(buffer != NULL);

    return apr_file_write_full(file, buffer, bytesToWrite, bytesWritten);
}

static
apr_status_t
FileFlush(
    void *opaque
    )
{
    apr_file_t *file = opaque;

    ASSERT(opaque != NULL);

    return apr_file_flush(file);
}

static
apr_status_t
FileClose(
    void *opaque
    )
{
    apr_file_t *file = opaque;

    ASSERT(opaque != NULL);

    return apr_file_close(file);
}

/*++

StreamCreateFile

    Creates a APR file I/O stream.

Arguments:
    file    - Pointer to a APR file handle.

    close   - If this argument is true, the APR file handle will be closed when
              the stream is closed. If this argument is false, the APR file handle
              will remain open when the stream is closed.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a STREAM structure.

    If the function fails, the return value is null.

--*/
STREAM *
StreamCreateFile(
    apr_file_t *file,
    bool_t close,
    apr_pool_t *pool
    )
{
    ASSERT(file != NULL);
    ASSERT(pool != NULL);

    return StreamCreate(file, FileRead, FileWrite, FileFlush,
        (close != FALSE) ? FileClose : NULL, pool);
}


typedef struct {
    encoding_t  readEnc;
    encoding_t  writeEnc;
} ENCODE_STREAM;

static
apr_status_t
EncodeRead(
    void *opaque,
    apr_byte_t *buffer,
    apr_size_t bytesToRead,
    apr_size_t *bytesRead
    )
{
    ASSERT(opaque != NULL);
    ASSERT(buffer != NULL);

    return APR_SUCCESS;
}

static
apr_status_t
EncodeWrite(
    void *opaque,
    const apr_byte_t *buffer,
    apr_size_t bytesToWrite,
    apr_size_t *bytesWritten
    )
{
    ASSERT(opaque != NULL);
    ASSERT(buffer != NULL);

    return APR_SUCCESS;
}

static
apr_status_t
EncodeFlush(
    void *opaque
    )
{
    ASSERT(opaque != NULL);

    return APR_SUCCESS;
}

static
apr_status_t
EncodeClose(
    void *opaque
    )
{
    ASSERT(opaque != NULL);

    return APR_SUCCESS;
}

STREAM *
StreamCreateEncoding(
    STREAM *stream,
    encoding_t readEnc,
    encoding_t writeEnc,
    apr_pool_t *pool
    )
{
    ASSERT(stream != NULL);
    ASSERT(pool != NULL);

    return APR_SUCCESS;
}
