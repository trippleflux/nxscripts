/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoCompress.c

Author:
    neoxed (neoxed@gmail.com) August 20, 2005

Abstract:
    This module implements an interface to Bzip2 and Zlib.

    compress adler32 <data>
      - Returns the Alder-32 checksum of "data".

    compress crc32 <data>
      - Returns the CRC-32 checksum of "data".

    compress compact [-level 0-9] <format> <data>
      - Compresses data in the specified format.
      - Supported formats: bzip2, gzip, zlib, and zlib-raw.
      - The '-level' switch sets the compression level, one by default.

    compress expand <format> <data>
      - Decompresses data from the specified format.
      - Supported formats: bzip2, gzip, zlib, and zlib-raw.
      - An error is raised if the given data cannot be decompressed.

    compress stream <format> <channel>
      - Create a compression layer for the specified channel.
      - Supported formats: bzip2, gzip, zlib, and zlib-raw.

--*/

#include <alcoExt.h>

//
// Bzip function prototypes.
//

static void *
BzipAlloc(
    void *opaque,
    int items,
    int size
    );

static void
BzipFree(
    void *opaque,
    void *address
    );

static int
BzipCompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int level
    );

static int
BzipDecompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj
   );

static void
BzipSetError(
    Tcl_Interp *interp,
    const char *message,
    int status
    );

//
// Zlib function prototypes.
//

static voidpf
ZlibAlloc(
    voidpf opaque,
    uInt items,
    uInt size
    );

static void
ZlibFree(
    voidpf opaque,
    voidpf address
    );

static int
ZlibCompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int level,
    int window
    );

static int
ZlibDecompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int window
    );

static void
ZlipSetError(
    Tcl_Interp *interp,
    const char *message,
    int status
    );

//
// Compression format table.
//

struct {
    char *name;     // Compression format name.
    int minLevel;   // Minimum compression level.
    int maxLevel;   // Maximum compression level.
    int window;     // Zlib window size (not used for bzip2).
} static const compressionFormats[] = {
    {"bzip2",    1, 9, 0},
    {"gzip",     0, 9, MAX_WBITS + 16},
    {"zlib",     0, 9, MAX_WBITS},
    {"zlib-raw", 0, 9, -MAX_WBITS},
    {NULL}
};

enum {
    TYPE_BZIP2 = 0,
    TYPE_GZIP,
    TYPE_ZLIB,
    TYPE_ZLIBRAW
};


/*++

bz_internal_error

    If Bzip is compiled stdio-free, this function is called when an
    internal error occurs within Bzip.

Arguments:
    errorCode - Identifier associated with the internal error.

Return Value:
    None.

--*/
#ifdef BZ_NO_STDIO
void
bz_internal_error(
    int errorCode
    )
{
#ifdef DEBUG
    printf("Bzip internal error: %d\n", errorCode);
#endif
}
#endif

/*++

BzipAlloc

    Allocates an array in memory.

Arguments:
    opaque  - Not used.

    items   - Number of items to allocate.

    size    - Size of each element, in bytes.

Return Value:
    If the function succeeds, the return value is a pointer to the allocated
    memory block. If the function fails, the return value is NULL.

Remarks:
    This function provides Bzip access to Tcl's memory allocation system.

--*/
static void *
BzipAlloc(
    void *opaque,
    int items,
    int size
    )
{
    return (void *)attemptckalloc(items * size);
}

/*++

BzipFree

    Frees a block of memory allocated by BzipAlloc.

Arguments:
    opaque  - Not used.

    address - Pointer to the memory block to be freed.

Return Value:
    None.

Remarks:
    This function provides Bzip access to Tcl's memory allocation system.

--*/
static void
BzipFree(
    void *opaque,
    void *address
    )
{
    if (address != NULL) {
        ckfree((char *)address);
    }
}

/*++

BzipCompressObj

    Compresses data using the Bzip2 compression algorithm.

Arguments:
    sourceObj - Pointer to a Tcl object containing the data to be compressed.

    destObj   - Pointer to a Tcl object to receive the compressed data.

    level     - Compression level.

Return Value:
    A Bzip2 status code; BZ_OK is returned if successful.

--*/
static int
BzipCompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int level
    )
{
    bz_stream stream;
    int status;
    unsigned int destLength;

    //
    // The bzalloc, bzfree, and opaque data structure members
    // must be initialised prior to calling BZ2_bzCompressInit().
    //
    stream.bzalloc = BzipAlloc;
    stream.bzfree  = BzipFree;
    stream.opaque  = NULL;

    status = BZ2_bzCompressInit(&stream, level, 0, 0);
    if (status != BZ_OK) {
        return status;
    }

    stream.next_in = (char *)Tcl_GetByteArrayFromObj(sourceObj, (int*)&stream.avail_in);

    //
    // According to the Bzip2 documentation, the recommended buffer size
    // is 1% larger than the uncompressed data, plus 600 additional bytes.
    //
    stream.avail_out = destLength = (unsigned int)((double)stream.avail_in * 1.01) + 600;
    stream.next_out  = (char *)Tcl_SetByteArrayLength(destObj, stream.avail_out);

    status = BZ2_bzCompress(&stream, BZ_FINISH);
    BZ2_bzCompressEnd(&stream);

    if (status == BZ_STREAM_END) {
        // Update the object's length.
        destLength -= stream.avail_out;
        Tcl_SetByteArrayLength(destObj, (int)destLength);
        return BZ_OK;
    }

    return (status == BZ_FINISH_OK) ? BZ_OUTBUFF_FULL : status;
}

/*++

BzipDecompressObj

    Decompresses Bzip2 compressed data.

Arguments:
    sourceObj - Pointer to a Tcl object containing the data to be decompressed.

    destObj   - Pointer to a Tcl object to receive the decompressed data.

Return Value:
    A Bzip2 status code; BZ_OK is returned if successful.

--*/
static int
BzipDecompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj
    )
{
    bz_stream stream;
    char *dest;
    int status;
    unsigned int destLength;
    unsigned int factor;
    unsigned int sourceLength;
    Tcl_WideUInt totalOut;

    stream.next_in = (char *)Tcl_GetByteArrayFromObj(sourceObj, (int *)&sourceLength);
    if (sourceLength < 3) {
        // The Bzip2 header is at least 3 characters in length, 'BZh'.
        return BZ_DATA_ERROR_MAGIC;
    }

    //
    // The bzalloc, bzfree, and opaque data structure members
    // must be initialised prior to calling BZ2_bzDecompressInit().
    //
    stream.bzalloc = BzipAlloc;
    stream.bzfree  = BzipFree;
    stream.opaque  = NULL;

    status = BZ2_bzDecompressInit(&stream, 0, 0);
    if (status != BZ_OK) {
        return status;
    }

    stream.avail_in = sourceLength;

    for (factor = 1; factor < 20; factor++) {
        // Double the destination buffer size each attempt.
        destLength = sourceLength * (1 << factor);
        dest = (char *)Tcl_SetByteArrayLength(destObj, (int)destLength);

        totalOut = ((Tcl_WideUInt)stream.total_out_hi32 << 32) + stream.total_out_lo32;
        stream.next_out  = dest + totalOut;
        stream.avail_out = destLength - (unsigned int)totalOut;

        //
        // BZ2_bzDecompress() returns:
        // - BZ_STREAM_END if the logical end of the stream has been reached.
        // - BZ_OK if the decompression was successful but there is remaining input data.
        // - Otherwise an error has occurred while decompressing the data.
        //
        status = BZ2_bzDecompress(&stream);
        if (status != BZ_OK) {
            break;
        }

        //
        // If BZ2_bzDecompress() returns BZ_OK without exhausting the output
        // buffer, it's assumed we've unexpectedly reached the stream's end.
        //
        if (stream.avail_out > 0) {
            status = BZ_UNEXPECTED_EOF;
            break;
        }

        // Increase the destination buffer size and try again.
        status = BZ_OUTBUFF_FULL;
    }

    BZ2_bzDecompressEnd(&stream);

    if (status == BZ_STREAM_END) {
        // Update the object's length.
        destLength -= stream.avail_out;
        Tcl_SetByteArrayLength(destObj, (int)destLength);
        return BZ_OK;
    }

    return status;
}

/*++

BzipSetError

    Sets the interpreter's result to a human-readable error message.

Arguments:
    interp  - Interpreter to use for error reporting.

    message - Message describing the failed operation.

    status  - Bzip2 status code.

Return Value:
    None.

--*/
static void
BzipSetError(
    Tcl_Interp *interp,
    const char *message,
    int status
    )
{
    static const char *errors[] = {
        "success",              // BZ_OK, BZ_RUN_OK, BZ_FLUSH_OK, BZ_FINISH_OK, BZ_STREAM_END
        "out of sequence",      // BZ_SEQUENCE_ERROR   (-1)
        "invalid parameter",    // BZ_PARAM_ERROR      (-2)
        "insufficient memory",  // BZ_MEM_ERROR        (-3)
        "data error",           // BZ_DATA_ERROR       (-4)
        "invalid header",       // BZ_DATA_ERROR_MAGIC (-5)
        "I/O error",            // BZ_IO_ERROR         (-6)
        "unexpected EOF",       // BZ_UNEXPECTED_EOF   (-7)
        "output buffer full",   // BZ_OUTBUFF_FULL     (-8)
        "config error",         // BZ_CONFIG_ERROR     (-9)
        "unknown error"
    };

    if (status > 0) {
        status = 0;
    } else {
        status = -status;
        if (status >= ARRAYSIZE(errors)) {
            status = ARRAYSIZE(errors)-1;
        }
    }

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, message, errors[status], NULL);
}

/*++

ZlibAlloc

    Allocates an array in memory.

Arguments:
    opaque  - Not used.

    items   - Number of items to allocate.

    size    - Size of each element, in bytes.

Return Value:
    If the function succeeds, the return value is a pointer to the allocated
    memory block. If the function fails, the return value is NULL.

Remarks:
    This function provides Zlib access to Tcl's memory allocation system.

--*/
static voidpf
ZlibAlloc(
    voidpf opaque,
    uInt items,
    uInt size
    )
{
    return (voidpf)attemptckalloc((int)(items * size));
}

/*++

ZlibFree

    Frees a block of memory allocated by ZlibAlloc.

Arguments:
    opaque  - Not used.

    address - Pointer to the memory block to be freed.

Return Value:
    None.

Remarks:
    This function provides Zlib access to Tcl's memory allocation system.

--*/
static void
ZlibFree(
    voidpf opaque,
    voidpf address
    )
{
    ckfree((char *)address);
}

/*++

ZlibCompressObj

    Compresses data using the Zlib compression algorithm.

Arguments:
    sourceObj - Pointer to a Tcl object containing the data to be compressed.

    destObj   - Pointer to a Tcl object to receive the compressed data.

    level     - Compression level.

    window    - Maximum window size for Zlib.

Return Value:
    A Zlib status code; Z_OK is returned if successful.

--*/
static int
ZlibCompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int level,
    int window
    )
{
    int status;
    z_stream stream;

    //
    // The next_in, opaque, zalloc, and zfree data structure members
    // must be initialised prior to calling deflateInit2().
    //
    stream.next_in = Tcl_GetByteArrayFromObj(sourceObj, (int *)&stream.avail_in);
    stream.opaque  = NULL;
    stream.zalloc  = ZlibAlloc;
    stream.zfree   = ZlibFree;

    status = deflateInit2(&stream, level, Z_DEFLATED, window,
        MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (status != Z_OK) {
        return status;
    }

    stream.avail_out = deflateBound(&stream, stream.avail_in);

    //
    // deflateBound() does not always return a sufficient buffer size when
    // compressing data into the Gzip format. So this kludge will do...
    //
    if (window > 15) {
        stream.avail_out *= 2;
    }

    stream.next_out = Tcl_SetByteArrayLength(destObj, (int)stream.avail_out);

    //
    // The Z_FINISH flag instructs Zlib to compress all data in a single
    // pass, flush it to the output buffer, and return with Z_STREAM_END if
    // successful.
    //
    status = deflate(&stream, Z_FINISH);
    deflateEnd(&stream);

    if (status == Z_STREAM_END) {
        Tcl_SetByteArrayLength(destObj, (int)stream.total_out);
        return Z_OK;
    }

    return (status == Z_OK) ? Z_BUF_ERROR : status;
}

/*++

ZlibDecompressObj

    Decompresses Zlib compressed data.

Arguments:
    sourceObj - Pointer to a Tcl object containing the data to be decompressed.

    destObj   - Pointer to a Tcl object to receive the decompressed data.

    window    - Maximum window size for Zlib.

Return Value:
    A Zlib status code; Z_OK is returned if successful.

--*/
static int
ZlibDecompressObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int window
    )
{
    int status;
    uInt destLength;
    uInt factor;
    uInt sourceLength;
    unsigned char *dest;
    z_stream stream;

    //
    // The avail_in, next_in, opaque, zalloc, and zfree data structure
    // members must be initialised prior to calling inflateInit2().
    //
    stream.next_in = Tcl_GetByteArrayFromObj(sourceObj, (int *)&sourceLength);
    if (sourceLength < 1) {
        return Z_DATA_ERROR;
    }

    stream.avail_in = sourceLength;
    stream.opaque   = NULL;
    stream.zalloc   = ZlibAlloc;
    stream.zfree    = ZlibFree;

    status = inflateInit2(&stream, window);
    if (status != Z_OK) {
        return status;
    }

    // Double the destination buffer size each attempt.
    for (factor = 1; factor < 20; factor++) {
        destLength = sourceLength * (1 << factor);
        dest = Tcl_SetByteArrayLength(destObj, (int)destLength);

        stream.next_out  = dest + stream.total_out;
        stream.avail_out = destLength - stream.total_out;

        //
        // inflate() returns:
        // - Z_STREAM_END if all input data has been exhausted.
        // - Z_OK if the inflation was successful but there is remaining input data.
        // - Otherwise an error has occurred while inflating the data.
        //
        status = inflate(&stream, Z_SYNC_FLUSH);
        if (status != Z_OK) {
            break;
        }

        //
        // If inflate() returns Z_OK without exhausting the output buffer,
        // it's assumed we've unexpectedly reached the stream's end.
        //
        if (stream.avail_out > 0) {
            status = Z_STREAM_ERROR;
            break;
        }

        // Increase the destination buffer size and try again.
        status = Z_BUF_ERROR;
    }

    inflateEnd(&stream);

    if (status == Z_STREAM_END) {
        // Update the object's length.
        Tcl_SetByteArrayLength(destObj, (int)stream.total_out);
        return Z_OK;
    }

    return status;
}

/*++

ZlipSetError

    Sets the interpreter's result to a human-readable error message.

Arguments:
    interp  - Interpreter to use for error reporting.

    message - Message describing the failed operation.

    status  - Zlib status code.

Return Value:
    None.

--*/
static void
ZlipSetError(
    Tcl_Interp *interp,
    const char *message,
    int status
    )
{
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, message, zError(status), NULL);
}

/*++

CompressObjCmd

    This function provides the "compress" Tcl command.

Arguments:
    dummy  - Not used.

    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
int
CompressObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    static const char *options[] = {
        "adler32", "compact", "crc32", "expand", "stream", NULL
    };
    enum options {
        OPTION_ADLER32, OPTION_COMPACT, OPTION_CRC32, OPTION_EXPAND, OPTION_STREAM
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_ADLER32:
        case OPTION_CRC32: {
            int dataLength;
            unsigned long checksum = 0;
            unsigned char *data;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "data");
                return TCL_ERROR;
            }

            data = Tcl_GetByteArrayFromObj(objv[2], &dataLength);

            switch ((enum options) index) {
                case OPTION_ADLER32: {
                    checksum = adler32(0, NULL, 0);
                    checksum = adler32(checksum, data, dataLength);
                    break;
                }
                case OPTION_CRC32: {
                    checksum = crc32(0, NULL, 0);
                    checksum = crc32(checksum, data, dataLength);
                    break;
                }
                default: {
                    return TCL_ERROR;
                }
            }

            Tcl_SetLongObj(Tcl_GetObjResult(interp), (long)checksum);
            return TCL_OK;
        }
        case OPTION_COMPACT: {
            int level = 1; // Quickest compression level by default.
            int status;

            switch (objc) {
                case 4: {
                    break;
                }
                case 6: {
                    if (PartialSwitchCompare(objv[2], "-level")) {
                        if (Tcl_GetIntFromObj(interp, objv[3], &level) != TCL_OK) {
                            return TCL_ERROR;
                        }
                        break;
                    }
                }
                default: {
                    Tcl_WrongNumArgs(interp, 2, objv, "?-level int? format data");
                    return TCL_ERROR;
                }
            }

            if (Tcl_GetIndexFromObjStruct(interp, objv[objc-2], compressionFormats,
                sizeof(compressionFormats[0]), "format", TCL_EXACT, &index) != TCL_OK) {
                return TCL_ERROR;
            }

            //
            // Zlib accepts compression levels from 0 to 9 while
            // Bzip2 only accepts 1 to 9.
            //
            if (level < compressionFormats[index].minLevel ||
                level > compressionFormats[index].maxLevel) {
                char message[64];

#ifdef _WINDOWS
                StringCchPrintfA(message, ARRAYSIZE(message),
                    "%d\": must be between %d and %d",
                    level,
                    compressionFormats[index].minLevel,
                    compressionFormats[index].maxLevel);
#else // _WINDOWS
                snprintf(message, ARRAYSIZE(message),
                    "%d\": must be between %d and %d",
                    level,
                    compressionFormats[index].minLevel,
                    compressionFormats[index].maxLevel);
                message[ARRAYSIZE(message)-1] = '\0';
#endif // _WINDOWS

                Tcl_AppendResult(interp, "invalid compression level \"", message, NULL);
                return TCL_ERROR;
            }

            if (index == TYPE_BZIP2) {
                status = BzipCompressObj(objv[objc-1], Tcl_GetObjResult(interp),
                    level);

                if (status != BZ_OK) {
                    BzipSetError(interp, "unable to compact data: ", status);
                    return TCL_ERROR;
                }
            } else {
                status = ZlibCompressObj(objv[objc-1], Tcl_GetObjResult(interp),
                    level, compressionFormats[index].window);

                if (status != Z_OK) {
                    ZlipSetError(interp, "unable to compact data: ", status);
                    return TCL_ERROR;
                }
            }

            return TCL_OK;
        }
        case OPTION_EXPAND: {
            int status;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "format data");
                return TCL_ERROR;
            }

            if (Tcl_GetIndexFromObjStruct(interp, objv[2], compressionFormats,
                sizeof(compressionFormats[0]), "format", TCL_EXACT, &index) != TCL_OK) {
                return TCL_ERROR;
            }

            if (index == TYPE_BZIP2) {
                status = BzipDecompressObj(objv[3], Tcl_GetObjResult(interp));

                if (status != BZ_OK) {
                    BzipSetError(interp, "unable to expand data: ", status);
                    return TCL_ERROR;
                }
            } else {
                status = ZlibDecompressObj(objv[3], Tcl_GetObjResult(interp),
                    compressionFormats[index].window);

                if (status != Z_OK) {
                    ZlipSetError(interp, "unable to expand data: ", status);
                    return TCL_ERROR;
                }
            }

            return TCL_OK;
        }
        case OPTION_STREAM: {
            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "format channel");
                return TCL_ERROR;
            }

            // TODO: channel support

            return TCL_OK;
        }
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
