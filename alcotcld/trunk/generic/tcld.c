/*
 * AlcoTcld - Alcoholicz Tcl daemon.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   tcld.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) July 17, 2005
 *
 * Abstract:
 *   Tcl scripting host.
 */

#include <tcld.h>

/*
 * TODO:
 * - NT service support (CtrlDispatcher and Handler).
 * - Set variable when running as a service (i.e. tcl_service 0/1).
 * - Add a Tcl command to fork() into the background?
 */

/*
 * main
 *
 *   Application entry point.
 *
 * Arguments:
 *   argc - Number of command-line arguments.
 *   argv - Array of pointers to strings that represent command-line arguments.
 *
 * Returns:
 *   If the function succeeds, the return value is zero. If the function
 *   fails, the return value is nonzero.
 *
 * Remarks:
 *   None.
 */
int main(int argc, char **argv)
{
    char argCount[20];
    char *argList;
    Tcl_Channel errorChannel;
    Tcl_DString argString;
    Tcl_Interp *interp;
    Tcl_Obj *scriptPath;
    int result;

    /* The first command-line argument must be a Tcl script. */
    if (argc < 2) {
        printf("Usage: %s <script file> [arguments]\n", argv[0]);
        return 1;
    }

    /* Initialise Tcl and create an interpreter. */
    Tcl_FindExecutable(argv[0]);
    interp = Tcl_CreateInterp();
    Tcl_InitMemory(interp);

    if (Tcl_Init(interp) != TCL_OK) {
        errorChannel = Tcl_GetStdChannel(TCL_STDERR);
        if (errorChannel) {
            Tcl_WriteChars(errorChannel, "Tcl initialisation failed: ", -1);
            Tcl_WriteObj(errorChannel, Tcl_GetObjResult(interp));
            Tcl_WriteChars(errorChannel, "\n", 1);
        }
    }

    /* Bail out if the intepreter was deleted. */
    if (Tcl_InterpDeleted(interp)) {
        result = 1;
        goto done;
    }

    /* Set the "argc", "argv", and "argv0" global variables. */
#ifdef _WINDOWS
    StringCchPrintfA(argCount, ARRAYSIZE(argCount), "%d", argc-1);
#else
    snprintf(argCount, ARRAYSIZE(argCount), "%d", argc-1);
    argCount[ARRAYSIZE(argCount)-1] = '\0';
#endif /* _WINDOWS */
    Tcl_SetVar(interp, "argc", argCount, TCL_GLOBAL_ONLY);

    argList = Tcl_Merge(argc-1, (CONST char **) argv+1);
    Tcl_ExternalToUtfDString(NULL, argList, -1, &argString);
    Tcl_SetVar(interp, "argv", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);
    ckfree(argList);

    Tcl_ExternalToUtfDString(NULL, argv[0], -1, &argString);
    Tcl_SetVar(interp, "argv0", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);

    /* Evaluate the start-up script in the created interpreter. */
    scriptPath = Tcl_NewStringObj(argv[1], -1);
    Tcl_IncrRefCount(scriptPath);

    if (Tcl_FSEvalFile(interp, scriptPath) == TCL_OK) {
        result = 0;
    } else {
        errorChannel = Tcl_GetStdChannel(TCL_STDERR);

        /* Write the contents of the "errorInfo" variable to stderr. */
        if (errorChannel) {
            Tcl_WriteChars(errorChannel, "Script evaluation failed: ", -1);
            Tcl_WriteObj(errorChannel, Tcl_GetVar2Ex(interp, "errorInfo",
                NULL, TCL_GLOBAL_ONLY));
            Tcl_WriteChars(errorChannel, "\n", 1);
        }
        result = 1;
    }

    Tcl_DecrRefCount(scriptPath);

    /* Clean-up before exit. */
    done:

    /* Delete the interpreter if it still exists. */
    if (!Tcl_InterpDeleted(interp)) {
        Tcl_DeleteInterp(interp);
    }

    Tcl_Finalize();
    return result;
}
