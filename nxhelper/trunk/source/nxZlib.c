/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxZlib.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Implements several zlib functions as Tcl commands; such as the Adler32
 *   and CRC32 checksums, zlib compression, and gzip/zlib decompression.
 *
 *   Tcl Commands:
 *     ::nx::zlib adler32 <data>
 *       - Returns the Alder32 checksum of "data".
 *
 *     ::nx::zlib crc32 <data>
 *       - Returns the CRC32 checksum of "data".
 *
 *     ::nx::zlib compress [-level 0-9] <format> <data>
 *       - Compresses data in the specified format.
 *       - Supported formats: gzip, zlib, and zlib-raw.
 *       - The "-level" switch sets the compression level, one by default.
 *
 *     ::nx::zlib decompress <format> <data>
 *       - Decompresses data from the specified format.
 *       - Supported formats: gzip, zlib, and zlib-raw.
 *       - An error is raised if the given data cannot be decompressed.
 */

#include <nxHelper.h>

/*
 * Zlib function prototypes.
 */

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

/*
 * Compression format table.
 */

struct {
    char *name;
    int window;
} static const compressionFormats[] = {
    {"gzip",     MAX_WBITS + 16},
    {"zlib",     MAX_WBITS},
    {"zlib-raw", -MAX_WBITS},
    {NULL}
};


/*
 * ZlibAlloc
 *
 *   Allocates an array in memory.
 *
 * Arguments:
 *   opaque  - Not used.
 *   items   - Number of items to allocate.
 *   size    - Size of each element, in bytes.
 *
 * Return Value:
 *   If the function succeeds, the return value is a pointer to the allocated
 *   memory block. If the function fails, the return value is NULL.
 *
 * Remarks:
 *   This function provides Zlib access to Tcl's memory allocation system.
 */
static voidpf
ZlibAlloc(
    voidpf opaque,
    uInt items,
    uInt size
    )
{
    return (voidpf)attemptckalloc((int)(items * size));
}

/*
 * ZlibFree
 *
 *   Frees a block of memory allocated by ZlibAlloc.
 *
 * Arguments:
 *   opaque  - Not used.
 *   address - Pointer to the memory block to be freed.
 *
 * Return Value:
 *   None.
 *
 * Remarks:
 *   This function provides Zlib access to Tcl's memory allocation system.
 */
static void
ZlibFree(
    voidpf opaque,
    voidpf address
    )
{
    ckfree((char *)address);
}

/*
 * ZlibCompressObj
 *
 *   Compresses data using the Zlib compression algorithm.
 *
 * Arguments:
 *   sourceObj - Pointer to a Tcl object containing the data to be compressed.
 *   destObj   - Pointer to a Tcl object to receive the compressed data.
 *   level     - Compression level.
 *   window    - Maximum window size for Zlib.
 *
 * Return Value:
 *   A Zlib status code; Z_OK is returned if successful.
 */
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

    assert(sourceObj != NULL);
    assert(destObj   != NULL);

    /*
     * The next_in, opaque, zalloc, and zfree data structure members
     * must be initialised prior to calling deflateInit2().
     */
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

    /*
     * deflateBound() does not always return a sufficient buffer size when
     * compressing data into the Gzip format. So this kludge will do...
     */
    if (window > 15) {
        stream.avail_out *= 2;
    }

    stream.next_out = Tcl_SetByteArrayLength(destObj, (int)stream.avail_out);

    /*
     * The Z_FINISH flag instructs Zlib to compress all data in a single
     * pass, flush it to the output buffer, and return with Z_STREAM_END if
     * successful.
     */
    status = deflate(&stream, Z_FINISH);
    deflateEnd(&stream);

    if (status == Z_STREAM_END) {
        Tcl_SetByteArrayLength(destObj, (int)stream.total_out);
        return Z_OK;
    }

    return (status == Z_OK) ? Z_BUF_ERROR : status;
}

/*
 * ZlibDecompressObj
 *
 *   Decompresses Zlib compressed data.
 *
 * Arguments:
 *   sourceObj - Pointer to a Tcl object containing the data to be decompressed.
 *   destObj   - Pointer to a Tcl object to receive the decompressed data.
 *   window    - Maximum window size for Zlib.
 *
 * Return Value:
 *   A Zlib status code; Z_OK is returned if successful.
 */
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

    assert(sourceObj != NULL);
    assert(destObj   != NULL);

    /*
     * The avail_in, next_in, opaque, zalloc, and zfree data structure
     * members must be initialised prior to calling inflateInit2().
     */
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

    /* Double the destination buffer size each attempt. */
    for (factor = 1; factor < 20; factor++) {
        destLength = sourceLength * (1 << factor);
        dest = Tcl_SetByteArrayLength(destObj, (int)destLength);

        stream.next_out  = dest + stream.total_out;
        stream.avail_out = destLength - stream.total_out;

        /*
         * inflate() returns:
         * - Z_STREAM_END if all input data has been exhausted.
         * - Z_OK if the inflation was successful but there is remaining input data.
         * - Otherwise an error has occurred while inflating the data.
         */
        status = inflate(&stream, Z_SYNC_FLUSH);
        if (status != Z_OK) {
            break;
        }

        /*
         * If inflate() returns Z_OK without exhausting the output buffer,
         * it's assumed we've unexpectedly reached the stream's end.
         */
        if (stream.avail_out > 0) {
            status = Z_STREAM_ERROR;
            break;
        }

        /* Increase the destination buffer size and try again. */
        status = Z_BUF_ERROR;
    }

    inflateEnd(&stream);

    if (status == Z_STREAM_END) {
        /* Update the object's length. */
        Tcl_SetByteArrayLength(destObj, (int)stream.total_out);
        return Z_OK;
    }

    return status;
}

/*
 * ZlipSetError
 *
 *   Sets the interpreter's result to a human-readable error message.
 *
 * Arguments:
 *   interp  - Interpreter to use for error reporting.
 *   message - Message describing the failed operation.
 *   status  - Zlib status code.
 *
 * Return Value:
 *   None.
 */
static void
ZlipSetError(
    Tcl_Interp *interp,
    const char *message,
    int status
    )
{
    assert(interp  != NULL);
    assert(message != NULL);

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, message, zError(status), NULL);
}


/*
 * ZlibObjCmd
 *
 *	 This function provides the "::nx::zlib" Tcl command.
 *
 * Arguments:
 *   dummy  - Not used.
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 */
int
ZlibObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    static const char *options[] = {
        "adler32", "compress", "crc32", "decompress", NULL
    };
    enum optionIndices {
        OPTION_ADLER32 = 0, OPTION_COMPRESS, OPTION_CRC32, OPTION_DECOMPRESS
    };

    /* Check arguments. */
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
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

            switch ((enum optionIndices) index) {
                case OPTION_ADLER32: {
                    checksum = adler32(0, Z_NULL, 0);
                    checksum = adler32(checksum, data, dataLength);
                    break;
                }
                case OPTION_CRC32: {
                    checksum = crc32(0, Z_NULL, 0);
                    checksum = crc32(checksum, data, dataLength);
                    break;
                }
            }

            Tcl_SetLongObj(Tcl_GetObjResult(interp), (long)checksum);
            return TCL_OK;
        }
        case OPTION_COMPRESS: {
            int level = Z_BEST_SPEED;  /* Use the quickest compression level by default. */
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

            status = ZlibCompressObj(objv[objc-1], Tcl_GetObjResult(interp),
                level, compressionFormats[index].window);

            if (status != Z_OK) {
                ZlipSetError(interp, "unable to compact data: ", status);
                return TCL_ERROR;
            }
            return TCL_OK;
        }
        case OPTION_DECOMPRESS: {
            int status;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "format data");
                return TCL_ERROR;
            }

            if (Tcl_GetIndexFromObjStruct(interp, objv[2], compressionFormats,
                    sizeof(compressionFormats[0]), "format", TCL_EXACT, &index) != TCL_OK) {
                return TCL_ERROR;
            }

            status = ZlibDecompressObj(objv[3], Tcl_GetObjResult(interp),
                compressionFormats[index].window);

            if (status != Z_OK) {
                ZlipSetError(interp, "unable to expand data: ", status);
                return TCL_ERROR;
            }
            return TCL_OK;
        }
    }

    /* This point is never reached. */
    assert(0);
    return TCL_ERROR;
}
