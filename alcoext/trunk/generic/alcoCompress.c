/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoCompress.c

Author:
    neoxed (neoxed@gmail.com) August 20, 2005

Abstract:
    This module implements an interface to bzip2 and zlib.

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
ZlibDeflateObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int level,
    int window
    );

static int
ZlibInflateObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int window
    );

//
// Compression format table.
//

struct {
    char *name; // Compression format name.
    int window; // Zlib window size (not used for bzip2).
} static const compressionFormats[] = {
    {"bzip2",    0},
    {"gzip",     MAX_WBITS + 16},
    {"zlib",     MAX_WBITS},
    {"zlib-raw", -MAX_WBITS},
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

    Frees a block of memory allocated by ZlibAlloc.

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

ZlibDeflateObj

    Compresses data using the zlib compression algorithm.

Arguments:
    sourceObj - Pointer to a Tcl object containing the data to be compressed.

    destObj   - Pointer to a Tcl object to receive the compressed data.

    level     - Compression level.

    window    - Maximum window size for zlib.

Return Value:
    A zlib status code; Z_OK is returned if successful.

--*/
static int
ZlibDeflateObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int level,
    int window
    )
{
    int status;
    z_stream stream;

    stream.next_in = Tcl_GetByteArrayFromObj(sourceObj, &(stream.avail_in));

    //
    // The opaque, zalloc, and zfree struct members must
    // be initialised before calling deflateInit2().
    //
    stream.opaque = (voidpf) NULL;
    stream.zalloc = ZlibAlloc;
    stream.zfree  = ZlibFree;

    // The stream must be initialised prior to calling deflateBound().
    status = deflateInit2(&stream, level, Z_DEFLATED, window,
        MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (status != Z_OK) {
        return status;
    }

    stream.avail_out = deflateBound(&stream, stream.avail_in);

    //
    // deflateBound() does not always return a sufficient buffer size when
    // compressing data into the gzip format. So, this kludge will do...
    //
    if (window > 15) {
        stream.avail_out *= 2;
    }

    stream.next_out = Tcl_SetByteArrayLength(destObj, stream.avail_out);

    //
    // The Z_FINISH flag instructs zlib to compress all data in a single
    // pass, flush it to the output buffer, and return with Z_STREAM_END if
    // successful.
    //
    status = deflate(&stream, Z_FINISH);
    if (status != Z_STREAM_END) {
        deflateEnd(&stream);
        return (status == Z_OK) ? Z_BUF_ERROR : status;
    }

    status = deflateEnd(&stream);
    Tcl_SetByteArrayLength(destObj, (status == Z_OK ? stream.total_out : 0));

    return status;
}

/*++

ZlibInflateObj

    Decompresses gzip or zlib compressed data.

Arguments:
    sourceObj - Pointer to a Tcl object containing the data to be decompressed.

    destObj   - Pointer to a Tcl object to receive the decompressed data.

    window    - Maximum window size for zlib.

Return Value:
    A zlib status code; Z_OK is returned if successful.

--*/
static int
ZlibInflateObj(
    Tcl_Obj *sourceObj,
    Tcl_Obj *destObj,
    int window
    )
{
    int bufferFactor;
    int bufferLength;
    int dataLength;
    int status;
    unsigned char *buffer;
    z_stream stream;

    stream.next_in = Tcl_GetByteArrayFromObj(sourceObj, &dataLength);
    if (dataLength < 1) {
        return Z_DATA_ERROR;
    }

    //
    // The opaque, zalloc, and zfree struct members must
    // be initialised before calling inflateInit2().
    //
    stream.opaque = (voidpf) NULL;
    stream.zalloc = ZlibAlloc;
    stream.zfree  = ZlibFree;
    stream.avail_in = dataLength;

    status = inflateInit2(&stream, window);
    if (status != Z_OK) {
        return status;
    }

    // Increase the buffer size by a factor of two each attempt.
    for (bufferFactor = 1; bufferFactor < 20; bufferFactor++) {
        bufferLength = dataLength * (1 << bufferFactor);
        buffer = Tcl_SetByteArrayLength(destObj, bufferLength);

        stream.next_out  = buffer + stream.total_out;
        stream.avail_out = bufferLength - stream.total_out;

        status = inflate(&stream, Z_SYNC_FLUSH);

        // inflate() returns Z_STREAM_END when all input data has been processed.
        if (status == Z_STREAM_END) {
            status = Z_OK;
            break;
        }

        //
        // If inflate() returns Z_OK with avail_out at zero, it must be
        // called again with a larger buffer to process remaining data.
        //
        if (status == Z_OK && stream.avail_out == 0) {
            status = Z_BUF_ERROR;
        } else {
            inflateEnd(&stream);
            return (status == Z_OK) ? Z_BUF_ERROR : status;
        }
    }

    inflateEnd(&stream);
    Tcl_SetByteArrayLength(destObj, (status == Z_OK ? stream.total_out : 0));

    return status;
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
            int level = Z_BEST_SPEED; // Use the quickest compression level by default.
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
                        if (level < 0 || level > 9) {
                            Tcl_AppendResult(interp, "invalid compression level \"",
                                Tcl_GetString(objv[3]), "\": must be between 0 and 9", NULL);
                            return TCL_ERROR;
                        }
                        break;
                    }
                }
                default: {
                    Tcl_WrongNumArgs(interp, 2, objv, "?-level 0-9? format data");
                    return TCL_ERROR;
                }
            }

            if (Tcl_GetIndexFromObjStruct(interp, objv[objc-2], compressionFormats,
                sizeof(compressionFormats[0]), "format", TCL_EXACT, &index) != TCL_OK) {
                return TCL_ERROR;
            }

            if (index == TYPE_BZIP2) {
                // TODO: bzip2 support
            } else {
                status = ZlibDeflateObj(objv[objc-1], Tcl_GetObjResult(interp),
                    level, compressionFormats[index].window);

                if (status != Z_OK) {
                    Tcl_ResetResult(interp);
                    Tcl_AppendResult(interp, "unable to compact data: ",
                        zError(status), NULL);
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
                // TODO: bzip2 support
            } else {
                status = ZlibInflateObj(objv[3], Tcl_GetObjResult(interp),
                    compressionFormats[index].window);

                if (status != Z_OK) {
                    Tcl_ResetResult(interp);
                    Tcl_AppendResult(interp, "unable to expand data: ",
                        zError(status), NULL);
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
