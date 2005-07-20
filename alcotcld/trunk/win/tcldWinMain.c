
#include <tcld.h>

static int    cmdArgc = 0;
static char **cmdArgv = NULL;

/* NT service related functions and global variables. */
static char *serviceName = NULL;
static HANDLE stopEvent  = NULL;
static SERVICE_STATUS_HANDLE serviceStatusHandle;

static void WINAPI ServiceMain(DWORD argc, char **argv);
static void WINAPI ServiceHandler(DWORD controlCode);
static void ServiceUpdateStatus(DWORD currentState, DWORD exitCode, DWORD waitHint);

/* Tcl related functions and global variables. */
static HANDLE tclThread = NULL;
static Tcl_ExitProc TclExitHandler;
static DWORD WINAPI TclThread(void *param);


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
    SERVICE_TABLE_ENTRYA serviceTable[] = {
        {"",   ServiceMain},
        {NULL, NULL}
    };

    /* Needed for TclThread. */
    cmdArgc = argc;
    cmdArgv = argv;

    if (!StartServiceCtrlDispatcherA(serviceTable)) {
        DWORD errorId = GetLastError();

        /*
         * If StartServiceCtrlDispatcher fails with ERROR_FAILED_SERVICE_CONTROLLER_CONNECT
         * the application was not started by the Service Control Manager, so we'll
         * continue to run as a console application.
         */
        if (errorId == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            Tcl_Interp *interp = TclInit(argc, argv, FALSE, NULL);
            if (interp == NULL) {
                return -1;
            }

            /* Wait forever... */
            for (;;) {
                Sleep(5000);
            }
        } else {
            DEBUGLOG("main: StartServiceCtrlDispatcher() failed with %lu.\n", errorId);
            return 1;
        }
    }

    /* Never reached. */
    return 0;
}

/*
 * ServiceMain
 *
 *   NT service entry point.
 *
 * Arguments:
 *   argc - Number of arguments in the argv array.
 *   argv - Array of pointers to strings that represent arguments passed to
 *          the process by the process that call StartService.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */
static void WINAPI ServiceMain(DWORD argc, char **argv)
{
    DWORD threadId;
    DEBUGLOG("ServiceMain: Service starting...\n", argc);

    serviceName = argv[0];
    serviceStatusHandle = RegisterServiceCtrlHandlerA(serviceName, ServiceHandler);

    /* Signal this event to notify threads to clean-up and exit. */
    stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (stopEvent == NULL) {
        DEBUGLOG("ServiceMain: CreateEvent() failed with %lu.\n", GetLastError());
        ServiceUpdateStatus(SERVICE_STOPPED, 1, 0);

    } else {
        DWORD result;

        DEBUGLOG("ServiceMain: Creating thread for Tcl...\n");
        tclThread = CreateThread(NULL, 0, TclThread, NULL, 0, &threadId);
        if (tclThread == NULL) {
            DEBUGLOG("ServiceMain: CreateThread() failed with %lu.\n", GetLastError());
            ServiceUpdateStatus(SERVICE_STOPPED, 1, 0);

        } else {
            DEBUGLOG("ServiceMain: Entering running state.\n");
            ServiceUpdateStatus(SERVICE_RUNNING, NO_ERROR, 0);

            /* Wait for the stop event to be signaled. */
            while (WaitForSingleObject(stopEvent, 1000) != WAIT_OBJECT_0) {
                DEBUGLOG("ServiceMain: Still waiting...\n");
            }
            DEBUGLOG("ServiceMain: Stop event signaled, waiting for Tcl thread to exit.\n");

            /* Wait for the Tcl thread to exit. */
            for (;;) {
                result = WaitForSingleObject(tclThread, 1000);
                if (result == WAIT_TIMEOUT) {
                    /* Retry... */
                    continue;
                }
                if (result == WAIT_FAILED) {
                    DEBUGLOG("ServiceMain: WaitForSingleObject() failed with %lu.\n", GetLastError());
                }
                break;
            }

            CloseHandle(stopEvent);
            CloseHandle(tclThread);

            DEBUGLOG("ServiceMain: Shutdown completed successfully.\n");
            ServiceUpdateStatus(SERVICE_STOPPED, NO_ERROR, 0);
        }
    }
}

/*
 * ServiceHandler
 *
 *   Service control callback, registered by ServiceMain.
 *
 * Arguments:
 *   controlCode - Control code, passed by the Service Control Manager.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */
static void WINAPI ServiceHandler(DWORD controlCode)
{
    DWORD currentState;
    DWORD waitHint;

    DEBUGLOG("ServiceHandler: Received control code %lu.\n", controlCode);

    switch (controlCode) {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP: {
            currentState = SERVICE_STOP_PENDING;
            /*
             * It shouldn't take more than five seconds for running threads to
             * clean-up and exit (including the time for Tcl to finalise).
             */
            waitHint = 5000;
            break;
        }
        default: {
            currentState = SERVICE_RUNNING;
            waitHint = 0;
            break;
        }
    }

    ServiceUpdateStatus(currentState, NO_ERROR, waitHint);

    if (currentState == SERVICE_STOP_PENDING) {
        /*
         * Signal all running threads to clean-up and exit since the
         * service has been requested to stop.
         */
        if (!SetEvent(stopEvent)) {
            DEBUGLOG("ServiceHandler: SetEvent() failed with %lu.\n", GetLastError());
        }
    }
}

/*
 * ServiceUpdateStatus
 *
 *   Updates the service's status, wraps around SetServiceStatus.
 *
 * Arguments:
 *   currentState - Current state of the service.
 *   exitCode     - Service exit code.
 *   waitHint     - Estimated time required for a operation, in milliseconds.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */
static void ServiceUpdateStatus(DWORD currentState, DWORD exitCode, DWORD waitHint)
{
    SERVICE_STATUS status;

    DEBUGLOG("ServiceUpdateStatus: State=%lu Exit=%lu Wait=%lu\n",
        currentState, exitCode, waitHint);

    /* Disable control requests until the service is running. */
    if (currentState == SERVICE_START_PENDING) {
        status.dwControlsAccepted = 0;
    } else {
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
    }

    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState            = currentState;
    status.dwWin32ExitCode           = exitCode;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = waitHint;

    if (!SetServiceStatus(serviceStatusHandle, &status)) {
        DEBUGLOG("ServiceUpdateStatus: SetServiceStatus() failed with %lu.\n", GetLastError());
    }
}


/*
 * TclExitHandler
 *
 *   Exit callback, executed by Tcl when the "exit" command is invoked.
 *
 * Arguments:
 *   dummy - Not used.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */
static void TclExitHandler(ClientData dummy)
{
    DEBUGLOG("TclExitHandler: Exit handler called, signaling stop event.\n");

    if (!SetEvent(stopEvent)) {
        DEBUGLOG("TclExitHandler: SetEvent() failed with %lu.\n", GetLastError());
    }
}

/*
 * TclThread
 *
 *   Thread started by ServiceMain which initialises a Tcl interpreter to
 *   evaluate the script file specified by the first command line argument.
 *
 * Arguments:
 *   param - Not used.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   This thread will terminate if the script file cannot be evaluated
 *   or if the stop event is signaled.
 */
static DWORD WINAPI TclThread(void *param)
{
    Tcl_Interp *interp;
    DEBUGLOG("TclThread: Starting...\n");

    interp = TclInit(cmdArgc, cmdArgv, TRUE, TclExitHandler);
    if (interp == NULL) {
        /*
         * TclInit will fail if the Tcl script cannot be evaluated, i.e.:
         * - No script file was specified in the command-line arguments.
         * - The specified file does not exist or cannot be read.
         * - The specified file contains Tcl syntax errors.
         */
        if (!SetEvent(stopEvent)) {
            DEBUGLOG("TclThread: SetEvent() failed with %lu.\n", GetLastError());
        }

        DEBUGLOG("TclThread: TclInit failed, exiting thread.\n");
        return 1;
    }

    while (WaitForSingleObject(stopEvent, 1000) != WAIT_OBJECT_0) {
        DEBUGLOG("TclThread: Still waiting...\n");
    }
    DEBUGLOG("TclThread: Stop event signaled, exiting thread.\n");

    /* Delete the interpreter if it still exists. */
    if (!Tcl_InterpDeleted(interp)) {
        Tcl_DeleteInterp(interp);
    }

    /*
     * Windows applications dynamically linked with Tcl aren't required to
     * call Tcl_Finalize() when exiting since Tcl will call this function
     * internally when the library is unloaded from the process.
     */
    return 0;
}
