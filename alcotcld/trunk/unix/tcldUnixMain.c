/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Unix Main

Author:
    neoxed (neoxed@gmail.com) Jul 20, 2005

Abstract:
    BSD/Linux/UNIX application entry point.

--*/

#include <tcld.h>

/*++

main

    Application entry point.

Arguments:
    argc    - Number of command-line arguments.
    argv    - Array of pointers to strings that represent command-line arguments.

Return Value:
    If the function succeeds, the return value is zero. If the function
    fails, the return value is nonzero.

--*/
int
main(
    int argc,
    char **argv
    )
{
    int background;
    char *currentPath;
    Tcl_Interp *interp;
    Tcl_Obj *resultObj;

    DEBUGLOG("main: Starting...\n");

    // Change working directory to the image location.
    currentPath = strdup(argv[0]);
    if (currentPath != NULL) {
        char *p = strrchr(currentPath, '/');
        if (p != NULL) {
            *p = '\0';
            if (setcwd(p) != 0) {
                DEBUGLOG("main: Unable to change directory to %s: %s.\n",
                    currentPath, strerror(errno));
            }
        }
        free(currentPath);
    } else {
        DEBUGLOG("main: strdup() failed: %s.\n", strerror(errno));
    }

    interp = TclInit(argc, argv, FALSE, NULL);
    if (interp == NULL) {
        return 1;
    }

    //
    // The evaluated script must return a true boolean value (1/true/on)
    // in order for the process to be forked into the background.
    //
    resultObj = Tcl_GetObjResult(interp);

    if (Tcl_GetBooleanFromObj(interp, resultObj, &background) != TCL_OK) {
        TclLogError("Invalid return value:\n",
            Tcl_GetVar2Ex(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY));
        return 1;
    }

    if (background) {
        pid_t pid = fork();

        if (pid == -1) {
            fprintf(stderr, "Unable to fork process: %s\n", strerror(errno));
            return 1;
        } else if (pid != 0) {
            printf("Forked process into the background (PID: %d).\n", pid);
            return 0;
        }
    } else {
        fprintf(stderr, "Script returned %d, not forking process.\n", background);
    }

    // Wait forever.
    for (;;) {
        sleep(5);
    }

    return 0;
}
