/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoZlib.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 16, 2005
 *
 * Abstract:
 *   Implements several zlib functions as Tcl commands; such as the Adler32
 *   and CRC32 checksums, and gzip/zlib compression. The implementation idea
 *   was taken from Tcl TIP #234 (http://www.tcl.tk/cgi-bin/tct/tip/234.html).
 *
 *   Tcl Commands:
 *     zlib adler32 <data>
 *       - Returns the Alder32 checksum of "data".
 *
 *     zlib crc32 <data>
 *       - Returns the CRC32 checksum of "data".
 *
 *     zlib compress [-level 0-9] <data>
 *       - Compresses the given data in raw zlib format. The returned data is
 *         does not have the zlib header, trailer, or integrity checksums.
 *       - The '-level' switch sets the compression level, one by default.
 *
 *     zlib decompress <data>
 *       - Decompresses raw data as obtained from 'zlib compress'.
 *       - An error is raised if the given data cannot be decompressed.
 *
 *     zlib deflate [-level 0-9] <data>
 *       - Compress the given data in zlib format.
 *       - The '-level' switch sets the compression level, one by default.
 *
 *     zlib inflate <data>
 *       - Decompresses zlib data as obtained from 'zlib deflate'.
 *       - An error is raised if the given data cannot be decompressed.
 *
 *     zlib gzip [-level 0-9] <data>
 *       - Compress the given data in gzip format.
 *       - The '-level' switch sets the compression level, one by default.
 *
 *     zlib gunzip <data>
 *       - Decompresses gzip data as obtained from 'zlib gzip'.
 *       - An error is raised if the given data cannot be decompressed.
 */

#include <alcoExt.h>

static int ZlibDeflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj, int level, int window);
static int ZlibInflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj, int window);


/*
 * ZlibDeflateObj
 *
 *   Compresses data using the zlib compression algorithm.
 *
 * Arguments:
 *   sourceObj - Pointer to a Tcl object containing the data to be compressed.
 *   destObj   - Pointer to a Tcl object to receive the compressed data.
 *   level     - Compression level.
 *   window    - Maximum window size for zlib.
 *
 * Returns:
 *   A zlib status code; Z_OK is returned if successful.
 *
 * Remarks:
 *   None.
 */

static int
ZlibDeflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj, int level, int window)
{
    int status;
    z_stream stream;

    stream.next_in = Tcl_GetByteArrayFromObj(sourceObj, &(stream.avail_in));

    /*
     * The opaque, zalloc, and zfree struct members must
     * be initialised to zero before calling deflateInit2().
     */
    stream.opaque = (voidpf) NULL;
    stream.zalloc = (alloc_func) NULL;
    stream.zfree  = (free_func) NULL;

    /* The stream must be initialised prior to calling deflateBound(). */
    status = deflateInit2(&stream, level, Z_DEFLATED, window,
        MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (status != Z_OK) {
        return status;
    }

    /*
     * deflateBound() returns the buffer size required for a single-pass deflation.
     * NOTE: This function was added in zlib v1.2x.
     */
    stream.avail_out = deflateBound(&stream, stream.avail_in);

    /*
     * deflateBound() does not always return a sufficient buffer size when
     * compressing data into the gzip format. So, this kludge will do...
     */
    if (window > 15) {
        stream.avail_out *= 2;
    }

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
 *   window    - Maximum window size for zlib.
 *
 * Returns:
 *   A zlib status code; Z_OK is returned if successful.
 *
 * Remarks:
 *   None.
 */

static int
ZlibInflateObj(Tcl_Obj *sourceObj, Tcl_Obj *destObj, int window)
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

    /*
     * The opaque, zalloc, and zfree struct members must
     * be initialised to zero before calling inflateInit2().
     */
    stream.opaque = (voidpf) NULL;
    stream.zalloc = (alloc_func) NULL;
    stream.zfree  = (free_func) NULL;
    stream.avail_in = dataLength;

    status = inflateInit2(&stream, window);
    if (status != Z_OK) {
        return status;
    }

    /* Increase the buffer size by a factor of two each attempt. */
    for (bufferFactor = 1; bufferFactor < 20; bufferFactor++) {
        bufferLength = dataLength * (1 << bufferFactor);
        buffer = Tcl_SetByteArrayLength(destObj, bufferLength);

        stream.next_out  = buffer + stream.total_out;
        stream.avail_out = bufferLength - stream.total_out;

        status = inflate(&stream, Z_SYNC_FLUSH);

        /* inflate() returns Z_STREAM_END when all input data has been processed. */
        if (status == Z_STREAM_END) {
            status = Z_OK;
            break;
        }

        /*
         * If inflate() returns Z_OK with avail_out at zero, it must be
         * called again with a larger buffer to process remaining data.
         */
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


/*
 * ZlibObjCmd
 *
 *   This function provides the "zlib" Tcl command.
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
    static const char *options[] = {
        "adler32", "compress", "crc32", "decompress",
        "deflate", "gunzip", "gzip", "inflate", NULL
    };
    enum options {
        OPTION_ADLER32, OPTION_COMPRESS, OPTION_CRC32, OPTION_DECOMPRESS,
        OPTION_DEFLATE, OPTION_GUNZIP, OPTION_GZIP, OPTION_INFLATE
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
        case OPTION_COMPRESS:
        case OPTION_GZIP:
        case OPTION_DEFLATE: {
            int level = Z_BEST_SPEED;  /* Use the quickest compression level by default. */
            int status;
            int window;

            switch (objc) {
                case 3: {
                    break;
                }
                case 5: {
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
                    Tcl_WrongNumArgs(interp, 2, objv, "?-level 0-9? data");
                    return TCL_ERROR;
                }
            }

            /* Each compression type uses a different window size. */
            switch ((enum options) index) {
                case OPTION_COMPRESS: {
                    /* Raw compressed data (no zlib header or trailer). */
                    window = -MAX_WBITS;
                    break;
                }
                case OPTION_GZIP: {
                    /* GZip compressed data. */
                    window = MAX_WBITS + 16;
                    break;
                }
                case OPTION_DEFLATE: {
                    /* Zlib compressed data. */
                    window = MAX_WBITS;
                    break;
                }
                default: {
                    return TCL_ERROR;
                }
            }

            status = ZlibDeflateObj(objv[objc-1], Tcl_GetObjResult(interp), level, window);

            if (status != Z_OK) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "unable to ", options[index], " data: ",
                    zError(status), NULL);
                return TCL_ERROR;
            }

            return TCL_OK;
        }
        case OPTION_DECOMPRESS:
        case OPTION_GUNZIP:
        case OPTION_INFLATE: {
            int status;
            int window;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "data");
                return TCL_ERROR;
            }

            /* Each compression type uses a different window size. */
            switch ((enum options) index) {
                case OPTION_DECOMPRESS: {
                    /* Raw compressed data (no zlib header or trailer). */
                    window = -MAX_WBITS;
                    break;
                }
                case OPTION_GUNZIP: {
                    /* GZip compressed data. */
                    window = MAX_WBITS + 16;
                    break;
                }
                case OPTION_INFLATE: {
                    /* Zlib compressed data. */
                    window = MAX_WBITS;
                    break;
                }
                default: {
                    return TCL_ERROR;
                }
            }

            status = ZlibInflateObj(objv[2], Tcl_GetObjResult(interp), window);

            if (status != Z_OK) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "unable to ", options[index], " data: ",
                    zError(status), NULL);
                return TCL_ERROR;
            }

            return TCL_OK;
        }
    }

    /* This point should never be reached. */
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
