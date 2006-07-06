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
    stream  - Pointer to a stream.

    buffer  - Pointer to the buffer that receives the data.

    length  - Pointer to the variable that specifies the number of bytes to read
              on entry; on exit, the number of bytes read.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
StreamRead(
    STREAM *stream,
    apr_byte_t *buffer,
    apr_size_t *length
    )
{
    ASSERT(stream != NULL);
    ASSERT(buffer != NULL);
    ASSERT(length != NULL);

    if (stream->readProc == NULL) {
        return APR_ENOTIMPL;
    }
    return stream->readProc(stream->opaque, buffer, length);
}

/*++

StreamWrite

    Writes data to an I/O stream.

Arguments:
    stream  - Pointer to a stream.

    buffer  - Pointer to the buffer containing the data to be written.

    length  - Pointer to the variable that specifies the number of bytes to write
              on entry; on exit, the number of bytes written.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
StreamWrite(
    STREAM *stream,
    const apr_byte_t *buffer,
    apr_size_t *length
    )
{
    ASSERT(stream != NULL);
    ASSERT(buffer != NULL);
    ASSERT(length != NULL);

    if (stream->writeProc == NULL) {
        return APR_ENOTIMPL;
    }
    return stream->writeProc(stream->opaque, buffer, length);
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
        case CONSOLE_STDIN:
            status = apr_file_open_stdin(&file, pool);
            break;
        case CONSOLE_STDOUT:
            status = apr_file_open_stdout(&file, pool);
            break;
        case CONSOLE_STDERR:
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
    apr_size_t *length
    )
{
    apr_status_t status = APR_SUCCESS;
    DWORD bytesRead;
    HANDLE console = opaque;

    // TODO: Convert from UTF-8 to UCS-2
    if (!ReadConsoleW(console, buffer, (DWORD)length, &bytesRead, NULL)) {
        status = apr_get_os_error();
    }

    *length = (apr_size_t)bytesRead;
    return APR_SUCCESS;
}

static
apr_status_t
TextConsoleWrite(
    void *opaque,
    const apr_byte_t *buffer,
    apr_size_t *length
    )
{
    apr_status_t status = APR_SUCCESS;
    DWORD bytesWritten;
    HANDLE console = opaque;

    // TODO: Convert from UTF-8 to UCS-2
    if (!WriteConsoleW(console, buffer, (DWORD)length, &bytesWritten, NULL)) {
        status = apr_get_os_error();
    }

    *length = (apr_size_t)bytesWritten;
    return status;
}

static
apr_status_t
TextConsoleFlush(
    void *opaque
    )
{
    HANDLE console = opaque;

    if (!FlushFileBuffers(console)) {
        return apr_get_os_error();
    }
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
        case CONSOLE_STDIN:
            type = STD_INPUT_HANDLE;
            break;
        case CONSOLE_STDOUT:
            type = STD_OUTPUT_HANDLE;
            break;
        case CONSOLE_STDERR:
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
    // TODO:
    // See if there's anything special when writing
    // UTF-8 text to stdout/stderr on *nix systems.
    return StreamCreateBinaryConsole(console, pool);
#endif // WINDOWS
}


static
apr_status_t
FileRead(
    void *opaque,
    apr_byte_t *buffer,
    apr_size_t *length
    )
{
    apr_file_t *file = opaque;

    return apr_file_read_full(file, buffer, *length, length);
}

static
apr_status_t
FileWrite(
    void *opaque,
    const apr_byte_t *buffer,
    apr_size_t *length
    )
{
    apr_file_t *file = opaque;

    return apr_file_write_full(file, buffer, *length, length);
}

static
apr_status_t
FileFlush(
    void *opaque
    )
{
    apr_file_t *file = opaque;

    return apr_file_flush(file);
}

static
apr_status_t
FileClose(
    void *opaque
    )
{
    apr_file_t *file = opaque;

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
        (close == TRUE) ? FileClose : NULL, pool);
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
    apr_size_t *length
    )
{
}

static
apr_status_t
EncodeWrite(
    void *opaque,
    const apr_byte_t *buffer,
    apr_size_t *length
    )
{
}

static
apr_status_t
EncodeFlush(
    void *opaque
    )
{
}

static
apr_status_t
EncodeClose(
    void *opaque
    )
{
}

STREAM *
StreamCreateEncoding(
    STREAM *stream,
    encoding_t readEnc,
    encoding_t writeEnc,
    apr_pool_t *pool
    )
{
}
