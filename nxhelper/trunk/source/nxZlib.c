/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
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
 *     ::nx::zlib deflate [-level 0-9] <data>
 *       - Compresses "data" using the zlib compression algorithm.
 *       - The -level switch sets the compression level, "fast" (1) by default.
 *       - An error is raised if the data cannot be compressed.
 *
 *     ::nx::zlib inflate <data>
 *       - Inflates gzip or zlib compressed data.
 *       - An error is raised if the data cannot be decompressed.
 */

#include <nxHelper.h>
#include <zlib.h>

static int ZlibDeflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj, int level);
static int ZlibInflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj);


/*
 * ZlibDeflateObj
 *
 *   Compresses data using the zlib compression algorithm.
 *
 * Arguments:
 *   sourceObj - Pointer to a Tcl object containing the data to be compressed.
 *   destObj   - Pointer to a Tcl object to receive the compressed data.
 *   level     - Compression level.
 *
 * Returns:
 *   A zlib status code; Z_OK is returned if successful.
 *
 * Remarks:
 *   None.
 */

static int
ZlibDeflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj, int level)
{
    int status;
    z_stream stream;

    stream.next_in = Tcl_GetByteArrayFromObj(sourceObj, &(stream.avail_in));

    /*
     * The opaque, zalloc, and zfree struct members must
     * be initialized to zero before calling deflateInit().
     */
    stream.opaque = (voidpf) Z_NULL;
    stream.zalloc = (alloc_func) Z_NULL;
    stream.zfree  = (free_func) Z_NULL;

    /* The stream must be initialized prior to calling deflateBound(). */
    status = deflateInit(&stream, level);
    if (status != Z_OK) {
        return status;
    }

    /*
     * deflateBound() returns the buffer size required for a single-pass deflation.
     * NOTE: This function was added in zlib v1.2x.
     */
    stream.avail_out = deflateBound(&stream, stream.avail_in);
    stream.next_out = Tcl_SetByteArrayLength(destObj, stream.avail_out);

    /*
     * The Z_FINISH flag instructs zlib to compress all data in a single
     * pass, flush it to the output buffer, and return with Z_STREAM_END if
     * successful.
     */
    status = deflate(&stream, Z_FINISH);
    if (status != Z_STREAM_END) {
        deflateEnd(&stream);
        return (status == Z_OK) ? Z_BUF_ERROR : status;
    }

    status = deflateEnd(&stream);
    Tcl_SetByteArrayLength(destObj, (status == Z_OK ? stream.total_out : 0));

    return status;
}


/*
 * ZlibInflateObj
 *
 *   Decompresses gzip or zlib compressed data.
 *
 * Arguments:
 *   sourceObj - Pointer to a Tcl object containing the data to be decompressed.
 *   destObj   - Pointer to a Tcl object to receive the decompressed data.
 *
 * Returns:
 *   A zlib status code; Z_OK is returned if successful.
 *
 * Remarks:
 *   None.
 */

static int
ZlibInflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj)
{
    int bufferFactor;
    int bufferLength;
    int bufferPrev = 0;
    int dataLength;
    int status;
    unsigned char *buffer;
    z_stream stream;

    stream.next_in = Tcl_GetByteArrayFromObj(sourceObj, &dataLength);

    /*
     * The opaque, zalloc, and zfree struct members must
     * be initialized to zero before calling inflateInit().
     */
    stream.opaque = (voidpf) Z_NULL;
    stream.zalloc = (alloc_func) Z_NULL;
    stream.zfree  = (free_func) Z_NULL;

    status = inflateInit(&stream);
    if (status != Z_OK) {
        return status;
    }

    /* Increase the buffer size by a factor of two each attempt. */
    for (bufferFactor = 1; bufferFactor < 16; bufferFactor++) {
        bufferLength = dataLength * (1 << bufferFactor);
        buffer = Tcl_SetByteArrayLength(destObj, bufferLength);

        stream.next_out  = buffer + bufferPrev;
        stream.avail_out = bufferLength - bufferPrev;

        status = inflate(&stream, Z_SYNC_FLUSH);

        /* inflate() returns Z_STREAM_END once all input data has been processed. */
        if (status == Z_STREAM_END) {
            status = Z_OK;
            break;
        }

        /*
         * If inflate() returns Z_OK and with avail_out at zero, it must be
         * called again with a larger buffer to process any pending data.
         */
        if (status == Z_OK && stream.avail_out == 0) {
            status = Z_BUF_ERROR;
        } else {
            inflateEnd(&stream);
            return (status == Z_OK) ? Z_BUF_ERROR : status;
        }

        bufferPrev = bufferLength;
    }

    inflateEnd(&stream);
    Tcl_SetByteArrayLength(destObj, (status == Z_OK ? stream.total_out : 0));

    return status;
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
 *
 * Remarks:
 *   None.
 */

int
ZlibObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int index;
    static const char *options[] = {"adler32", "crc32", "deflate", "inflate", NULL};
    enum options {OPTION_ADLER32, OPTION_CRC32, OPTION_DEFLATE, OPTION_INFLATE};

    if (objc < 3) {
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
        case OPTION_DEFLATE: {
            int level = Z_BEST_SPEED;  /* Use the quickest compression level by default. */
            int status;

            switch (objc) {
                case 3: {
                    break;
                }
                case 5: {
                    if (strcmp(Tcl_GetStringFromObj(objv[2], NULL), "-level") == 0) {
                        if (Tcl_GetIntFromObj(interp, objv[3], &level) != TCL_OK) {
                            return TCL_ERROR;
                        }
                        if (level < 0 || level > 9) {
                            Tcl_SetResult(interp, "invalid compression level: must be between 0 and 9", TCL_STATIC);
                            return TCL_ERROR;
                        }
                        break;
                    }
                }
                default: {
                    Tcl_WrongNumArgs(interp, 2, objv, "?-level 0-9? data");
                    return TCL_ERROR;
                }
            }

            status = ZlibDeflateObj(objv[objc-1], Tcl_GetObjResult(interp), level);

            if (status != Z_OK) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "unable to deflate data: ", zError(status), NULL);
                return TCL_ERROR;
            }

            return TCL_OK;
        }
        case OPTION_INFLATE: {
            int status;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "data");
                return TCL_ERROR;
            }

            status = ZlibInflateObj(objv[2], Tcl_GetObjResult(interp));

            if (status != Z_OK) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "unable to inflate data: ", zError(status), NULL);
                return TCL_ERROR;
            }

            return TCL_OK;
        }
    }

    /* This point should never be reached. */
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
