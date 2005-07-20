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

static void TclLogError(const char *message, Tcl_Obj *objPtr);


/*
 * DebugLog
 *
 *   Writes an entry to the applications debug log file.
 *
 * Arguments:
 *   format - Pointer to a buffer containing a printf-style format string.
 *   ...    - Arguments to insert into 'format'.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */
#if defined(DEBUG) || defined(_DEBUG)
void DebugLog(const char *format, ...)
{
    FILE *logHandle;
    va_list argList;
#ifdef _WINDOWS
    SYSTEMTIME now;
#else
    struct tm *now;
#endif /* _WINDOWS */

    va_start(argList, format);
    logHandle = fopen("Debug.log", "a");
    if (logHandle != NULL) {
#ifdef _WINDOWS
        GetSystemTime(&now);
        fprintf(logHandle, "%04d-%02d-%02d %02d:%02d:%02d [%04lu] ",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
            GetCurrentThreadId());
#else
        now = localtime(time(NULL));
        fprintf(logHandle, "%04d-%02d-%02d %02d:%02d:%02d - ",
            now->tm_year, now->tm_mon, now->tm_mday,
            now->tm_hour, now->tm_min, now->tm_sec);
#endif /* _WINDOWS */

        vfprintf(logHandle, format, argList);
        fclose(logHandle);
    }
    va_end(argList);
}
#endif /* DEBUG || _DEBUG */

/*
 * TclInit
 *
 *   Initialises a Tcl interpeter and evaluates the Tcl script in argv.
 *
 * Arguments:
 *   argc     - Number of command-line arguments.
 *   argv     - Array of pointers to strings that represent command-line arguments.
 *   service  - Boolean to indicate whether the process is running as an NT service.
 *   exitProc - Pointer to a Tcl exit handler function. This argument can be NULL.
 *
 * Returns:
 *   If the function succeeds, the return value is pointer to a Tcl interpeter
 *   structure. If the function fails, the return value is NULL.
 *
 * Remarks:
 *   None.
 */
Tcl_Interp *TclInit(int argc, char **argv, int service, Tcl_ExitProc *exitProc)
{
    char *argList;
    int success = 1;
    Tcl_DString argString;
    Tcl_Interp *interp;
    Tcl_Obj *intObj;

    /* The second command-line argument must be a Tcl script. */
    if (argc < 2) {
        DEBUGLOG("TclInit: Invalid command-line arguments.\n");
        fprintf(stderr, "Usage: %s <script file> [arguments]\n", argv[0]);
        return NULL;
    }

#ifdef _WINDOWS
    /* Redirect standard channels for Tcl. */
    if (service) {
        freopen("nul", "r", stdin);
        freopen("nul", "w", stdout);
        freopen("nul", "w", stderr);
    }
#endif

    /* Initialise Tcl and create an interpreter. */
    Tcl_FindExecutable(argv[0]);
    interp = Tcl_CreateInterp();
    Tcl_InitMemory(interp);

    /*
     * Source the init.tcl script. If this operation fails, we can
     * continue and function normally (for the most part anyway).
     */
    if (Tcl_Init(interp) != TCL_OK) {
        TclLogError("Tcl initialisation failed:\n", Tcl_GetObjResult(interp));
    }

    /* Set the "argc", "argv", and "argv0" global variables. */
    intObj = Tcl_NewIntObj(argc-1);
    Tcl_IncrRefCount(intObj);
    Tcl_SetVar2Ex(interp, "argc", NULL, intObj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(intObj);

    argList = Tcl_Merge(argc-1, (CONST char **) argv+1);
    Tcl_ExternalToUtfDString(NULL, argList, -1, &argString);
    Tcl_SetVar(interp, "argv", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);
    ckfree(argList);

    Tcl_ExternalToUtfDString(NULL, argv[0], -1, &argString);
    Tcl_SetVar(interp, "argv0", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);

    /* Set tcl_service, to indicate whether the process is running as a NT service. */
    intObj = Tcl_NewIntObj(service != 0);
    Tcl_IncrRefCount(intObj);
    Tcl_SetVar2Ex(interp, "tcl_service", NULL, intObj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(intObj);

    /*
     * Create an exit callback to handle unexpected exit requests, allowing
     * us to clean up. For example, if the script being evaluated invokes
     * the "exit" command.
     */
    if (exitProc != NULL) {
        Tcl_CreateExitHandler(exitProc, NULL);
    }

    if (Tcl_EvalFile(interp, argv[1]) != TCL_OK) {
        TclLogError("Script evaluation failed:\n",
            Tcl_GetVar2Ex(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY));

        /* Delete the interpreter if it still exists. */
        if (!Tcl_InterpDeleted(interp)) {
            Tcl_DeleteInterp(interp);
        }
        success = 0;
    }

    return (success != 0) ? interp : NULL;
}

/*
 * TclLogError
 *
 *   Displays an error message to stderr and logs it to Error.log.
 *
 * Arguments:
 *   message - Pointer to a buffer containing a string which explains the error.
 *   objPtr  - Pointer to a Tcl object containing the error text.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */
static void TclLogError(const char *message, Tcl_Obj *objPtr)
{
    Tcl_Channel channel;

    DEBUGLOG("TclLogError: %s%s\n", message, Tcl_GetString(objPtr));

    /* Write message to stderr. */
    channel = Tcl_GetStdChannel(TCL_STDERR);
    if (channel != NULL) {
        Tcl_WriteChars(channel, message, -1);
        Tcl_WriteObj(channel, objPtr);
        Tcl_WriteChars(channel, "\n", 1);
    }

    /* Write message to Error.log. */
    channel = Tcl_OpenFileChannel(NULL, "Error.log", "a", 0644);
    if (channel != NULL) {
        char timeStamp[64];
#ifdef _WINDOWS
        SYSTEMTIME now;
        GetSystemTime(&now);

        StringCchPrintfA(timeStamp, ARRAYSIZE(timeStamp), "%04d-%02d-%02d %02d:%02d:%02d ",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
#else
    struct tm *now;
        now = localtime(time(NULL));

        snprintf(timeStamp, ARRAYSIZE(timeStamp), "%04d-%02d-%02d %02d:%02d:%02d ",
            now->tm_year, now->tm_mon, now->tm_mday,
            now->tm_hour, now->tm_min, now->tm_sec);
       timeStamp[ARRAYSIZE(timeStamp)-1] = '\0';
#endif /* _WINDOWS */

        Tcl_WriteChars(channel, timeStamp, -1);
        Tcl_WriteChars(channel, message, -1);
        Tcl_WriteObj(channel, objPtr);
        Tcl_WriteChars(channel, "\n\n", 2);
        Tcl_Close(NULL, channel);
    }
}
