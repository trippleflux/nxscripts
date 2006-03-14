/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxMP3.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Implements an interface to retrieve information from MP3 files.
 *
 *   Tcl Commands:
 *     ::nx::mp3 <file> <varName>
 *      - Retrieves the ID3/MP3 header info for "file" and uses
 *        the array given by "varName" to store information.
 *      - An error is raised if the file is invalid or cannot be opened for reading.
 *      - Array Contents:
 *        bitrate   - Average audio bitrate, in Kbit/s.
 *        frames    - Number of frames.
 *        frequency - Audio frequency.
 *        layer     - MPEG audio layer.
 *        length    - Audio length, in seconds.
 *        mode      - Stereo mode.
 *        type      - Type of audio, CBR or VBR.
 *        version   - MPEG version.
 *        album     - Title of the recording.
 *        artist    - Artist or author's name.
 *        comment   - File comment.
 *        genre     - Genre or type of music.
 *        title     - Song title.
 *        track     - Track number (1.1 only).
 *        year      - Release year.
 *        id3       - ID3 tag version.
 *
 */

#include <nxHelper.h>


/*
 * Mp3ObjCmd
 *
 *	 This function provides the "::nx::mp3" Tcl command.
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
Mp3ObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int status = TCL_ERROR;
    MP3INFO MP3Info;
    TCHAR *filePath;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "file varName");
        return TCL_ERROR;
    }
    filePath = Tcl_GetTString(objv[1]);

    if (MP3OpenFile(filePath, &MP3Info)) {
        MP3TAG MP3Tag;

        if (MP3LoadHeader(&MP3Info)) {
            BOOL isTagged = MP3GetTag(&MP3Info, &MP3Tag);
            static char *unknown = "unknown";
            unsigned long i;
            Tcl_Obj *fieldObj;
            Tcl_Obj *valueObj;

            /* List of 'double' fields. */
            struct {
                char *field;
                double value;
            } doubleValues[] = {
                {"version",   (double)MP3GetVersion(&MP3Info)},
                {"id3",       isTagged ? (double)MP3Tag.Version : (double)0.0},
                NULL
            };

            /* List of 'long' fields. */
            struct {
                char *field;
                long value;
            } longValues[] = {
                {"bitrate",   MP3GetBitrate(&MP3Info)},
                {"frames",    MP3GetFrames(&MP3Info)},
                {"frequency", MP3GetFrequency(&MP3Info)},
                {"layer",     MP3GetLayer(&MP3Info)},
                {"length",    MP3GetLength(&MP3Info)},
                {"year",      isTagged ? MP3Tag.Year : 0},
                {"track",     isTagged ? MP3Tag.Track : 0},
                NULL
            };

            /* List of 'string' fields. */
            struct {
                char *field;
                char *value;
            } stringValues[] = {
                {"mode",      MP3GetMode(&MP3Info)},
                {"type",      MP3Info.IsVbr ? "VBR" : "CBR"},
                {"title",     isTagged ? MP3Tag.Title : unknown},
                {"artist",    isTagged ? MP3Tag.Artist : unknown},
                {"album",     isTagged ? MP3Tag.Album : unknown},
                {"genre",     isTagged ? MP3Tag.Genre : unknown},
                {"comment",   isTagged ? MP3Tag.Comment : unknown},
                NULL
            };

            /*
             * Tcl_ObjSetVar2() won't create a copy of the field object,
             * so the caller must free the object once finished with it.
             */
            fieldObj = Tcl_NewObj();
            Tcl_IncrRefCount(fieldObj);

/* Easier then repeating this... */
#define TCL_STORE_ARRAY(name, value)                                                      \
    valueObj = (value);                                                                   \
    Tcl_SetStringObj(fieldObj, (name), -1);                                               \
    if (Tcl_ObjSetVar2(interp, objv[2], fieldObj, valueObj, TCL_LEAVE_ERR_MSG) == NULL) { \
        Tcl_DecrRefCount(fieldObj);                                                       \
        return TCL_ERROR;                                                                 \
    }

            for (i = 0; doubleValues[i].field != NULL; i++) {
                TCL_STORE_ARRAY(doubleValues[i].field,
                    Tcl_NewDoubleObj(doubleValues[i].value));
            }

            for (i = 0; longValues[i].field != NULL; i++) {
                TCL_STORE_ARRAY(longValues[i].field,
                    Tcl_NewLongObj(longValues[i].value));
            }

            for (i = 0; stringValues[i].field != NULL; i++) {
                TCL_STORE_ARRAY(stringValues[i].field,
                    Tcl_NewStringObj(stringValues[i].value, -1));
            }

            Tcl_DecrRefCount(fieldObj);
            status = TCL_OK;
        } else {
            Tcl_AppendResult(interp, "unable to read \"", Tcl_GetString(objv[1]),
            "\": not a valid MP3 file", NULL);
        }

        MP3CloseFile(&MP3Info);
    } else {
        Tcl_AppendResult(interp, "unable to open \"", Tcl_GetString(objv[1]), "\": ",
            TclSetWinError(interp, GetLastError()), NULL);
    }

    return status;
}
