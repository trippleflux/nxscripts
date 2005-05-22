#include <nxHelper.h>

#if 0
// Tcl object types
#define TYPE_DOUBLE 0
#define TYPE_LONG   1
#define TYPE_STRING 2

typedef struct {
    PCHAR Field;
    SHORT Type;
    PVOID Value;
} ELEMENT_LIST;

ELEMENT_LIST Elements[] = {
    // Header Information
    {"bitrate",   TYPE_LONG,   MP3GetBitrate(&MP3Info)},
    {"frames",    TYPE_LONG,   MP3GetFrames(&MP3Info)},
    {"frequency", TYPE_LONG,   MP3GetFrequency(&MP3Info)},
    {"layer",     TYPE_LONG,   MP3GetLayer(&MP3Info)},
    {"length",    TYPE_LONG,   MP3GetLength(&MP3Info)},
    {"mode",      TYPE_STRING, MP3GetMode(&MP3Info)},
    {"type",      TYPE_STRING, MP3Info.IsVbr ? "VBR" : "CBR"},
    {"version",   TYPE_DOUBLE, MP3GetVersion(&MP3Info)},

    // ID3 Information
    {"title",     TYPE_STRING, Tagged ? MP3Tag.Title : Unknown},
    {"artist",    TYPE_STRING, Tagged ? MP3Tag.Artist : Unknown},
    {"album",     TYPE_STRING, Tagged ? MP3Tag.Album : Unknown},
    {"genre",     TYPE_STRING, Tagged ? MP3Tag.Genre : Unknown},
    {"comment",   TYPE_STRING, Tagged ? MP3Tag.Comment : Unknown},
    {"year",      TYPE_LONG,   Tagged ? MP3Tag.Year : 0},
    {"track",     TYPE_LONG,   Tagged ? MP3Tag.Track : 0},
    {"id3",       TYPE_DOUBLE, Tagged ? MP3Tag.Version : 0.0},
    {NULL,        0,           NULL}
};
#endif


INT TclMp3Cmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT ReturnValue = 0;
    MP3INFO MP3Info;
    PTCHAR FilePath;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "file varName");
        return TCL_ERROR;
    }
    FilePath = Tcl_GetTString(objv[1]);

    if (MP3OpenFile(FilePath, &MP3Info)) {
        MP3TAG MP3Tag;

        if (MP3LoadHeader(&MP3Info)) {
            BOOL Tagged = MP3GetTag(&MP3Info, &MP3Tag);
            PCHAR Unknown = "Unknown";
            ULONG i;

            Tcl_Obj *VarObj = Tcl_NewStringObj(Tcl_GetString(objv[2]), -1);
            Tcl_Obj *FieldObj = Tcl_NewObj();
            Tcl_Obj *ValueObj;

            // List of type 'double' fields.
            struct DoubleValues {
                PCHAR Field;
                double Value;
            } DoubleValues[] = {
                {"version",   (double)MP3GetVersion(&MP3Info)},
                {"id3",       Tagged ? (double)MP3Tag.Version : (double)0.0},
                NULL
            };

            // List of type 'long' fields.
            struct LongValues {
                PCHAR Field;
                LONG Value;
            } LongValues[] = {
                {"bitrate",   MP3GetBitrate(&MP3Info)},
                {"frames",    MP3GetFrames(&MP3Info)},
                {"frequency", MP3GetFrequency(&MP3Info)},
                {"layer",     MP3GetLayer(&MP3Info)},
                {"length",    MP3GetLength(&MP3Info)},
                {"year",      Tagged ? MP3Tag.Year : 0},
                {"track",     Tagged ? MP3Tag.Track : 0},
                NULL
            };

            // List of type 'string' fields.
            struct StringValues {
                PCHAR Field;
                PCHAR Value;
            } StringValues[] = {
                {"mode",      MP3GetMode(&MP3Info)},
                {"type",      MP3Info.IsVbr ? "VBR" : "CBR"},
                {"title",     Tagged ? MP3Tag.Title : Unknown},
                {"artist",    Tagged ? MP3Tag.Artist : Unknown},
                {"album",     Tagged ? MP3Tag.Album : Unknown},
                {"genre",     Tagged ? MP3Tag.Genre : Unknown},
                {"comment",   Tagged ? MP3Tag.Comment : Unknown},
                NULL
            };

            // Set all double values.
            for (i = 0; DoubleValues[i].Field != NULL; i++) {
                Tcl_SetStringObj(FieldObj, DoubleValues[i].Field, -1);
                ValueObj = Tcl_NewDoubleObj(DoubleValues[i].Value);

                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }
            }

            // Set all long values.
            for (i = 0; LongValues[i].Field != NULL; i++) {
                Tcl_SetStringObj(FieldObj, LongValues[i].Field, -1);
                ValueObj = Tcl_NewLongObj(LongValues[i].Value);

                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }
            }

            // Set all string values.
            for (i = 0; StringValues[i].Field != NULL; i++) {
                Tcl_SetStringObj(FieldObj, StringValues[i].Field, -1);
                ValueObj = Tcl_NewStringObj(StringValues[i].Value, -1);

                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }
            }

            ReturnValue = 1;
        }

        MP3CloseFile(&MP3Info);
    }

    Tcl_SetIntObj(Tcl_GetObjResult(interp), ReturnValue);
    return TCL_OK;
}
