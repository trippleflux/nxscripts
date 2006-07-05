/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Stream

Author:
    neoxed (neoxed@gmail.com) Jul 5, 2006

Abstract:
    I/O stream functions prototypes and structures.

--*/

#ifndef _STREAM_H_
#define _STREAM_H_

//
// Stream structure and callbacks
//

typedef struct STREAM STREAM;

typedef apr_status_t (STREAM_READ_PROC)(
    void *opaque,
    apr_byte_t *buffer,
    apr_size_t *length
    );

typedef apr_status_t (STREAM_WRITE_PROC)(
    void *opaque,
    const apr_byte_t *buffer,
    apr_size_t *length
    );

typedef apr_status_t (STREAM_FLUSH_PROC)(
    void *opaque
    );

typedef apr_status_t (STREAM_CLOSE_PROC)(
    void *opaque
    );

//
// Stream interface
//

STREAM *
StreamCreate(
    void *opaque,
    STREAM_READ_PROC *readProc,
    STREAM_WRITE_PROC *writeProc,
    STREAM_FLUSH_PROC *flushProc,
    STREAM_CLOSE_PROC *closeProc,
    apr_pool_t *pool
    );

apr_status_t
StreamRead(
    STREAM *stream,
    apr_byte_t *buffer,
    apr_size_t *length
    );

apr_status_t
StreamWrite(
    STREAM *stream,
    const apr_byte_t *buffer,
    apr_size_t *length
    );

apr_status_t
StreamFlush(
    STREAM *stream
    );

apr_status_t
StreamClose(
    STREAM *stream
    );

//
// Console stream
//

#define CONSOLE_STDIN   0
#define CONSOLE_STDOUT  1
#define CONSOLE_STDERR  2

STREAM *
StreamCreateConsole(
    int console,
    apr_pool_t *pool
    );

//
// File stream
//

STREAM *
StreamCreateFile(
    apr_file_t *file,
    bool_t close,
    apr_pool_t *pool
    );

//
// Encoding translation stream
//

STREAM *
StreamCreateEncoding(
    STREAM *stream,
    encoding_t readEnc,
    encoding_t writeEnc,
    apr_pool_t *pool
    );

#endif // _STREAM_H_
