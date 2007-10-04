/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    File Logging

Author:
    neoxed (neoxed@gmail.com) Sep 30, 2007

Abstract:
    Logging information to file.

*/

#include <base.h>
#include <logging.h>
#include <queue.h>

//
// Log structures and definitions
//

typedef struct LOG_ENTRY {
    STAILQ_ENTRY(LOG_ENTRY) link;   // Linked list of log entries
    SYSTEMTIME      time;           // Time the log entry was created
    CHAR            message[512];   // Message to log
    size_t          length;         // Length of the message, in characters
} LOG_ENTRY;

STAILQ_HEAD(LOG_QUEUE, LOG_ENTRY);
typedef struct LOG_QUEUE LOG_QUEUE;

#define LOG_STATUS_ACTIVE   0       // Log system is active
#define LOG_STATUS_SHUTDOWN 1       // Log system is shutting down
#define LOG_STATUS_INACTIVE 2       // Log system is inactive

//
// Log variables
//

static CRITICAL_SECTION logLock;    // Lock to protect access to the handle and queue
static HANDLE           logHandle;  // Handle to the log file
static LOG_QUEUE        logQueue;   // Queue of log entries to write
static volatile LONG    logStatus = LOG_STATUS_INACTIVE;


static INLINE LOG_ENTRY *GetSpareEntry(VOID)
{
    if (logStatus != LOG_STATUS_ACTIVE) {
        // Logging system must be active
        return NULL;
    }
    return MemAllocate(sizeof(LOG_ENTRY));
}

static INLINE LOG_ENTRY *GetQueueEntry(VOID)
{
    LOG_ENTRY *entry;

    EnterCriticalSection(&logLock);

    // Pop the first entry off the queue
    if (!STAILQ_EMPTY(&logQueue)) {
        entry = STAILQ_FIRST(&logQueue);
        STAILQ_REMOVE_HEAD(&logQueue, link);
    } else {
        entry = NULL;
    }

    LeaveCriticalSection(&logLock);
    return entry;
}

static VOID CCALL QueueWrite(VOID *context)
{
    CHAR        buffer[128];
    DWORD       written;
    LOG_ENTRY   *entry;
    size_t      remaining;

    UNREFERENCED_PARAMETER(context);

    // Log system must be inactive or shutting down (flush)
    if (logStatus != LOG_STATUS_ACTIVE && logStatus != LOG_STATUS_SHUTDOWN) {
        return;
    }

    // Process all queued log entries
    for (;;) {
        entry = GetQueueEntry();
        if (entry == NULL) {
            break;
        }

        // Format the time stamp
        StringCchPrintfExA(buffer, ELEMENT_COUNT(buffer), NULL, &remaining, 0,
            "%02d-%02d-%04d %02d:%02d:%02d ",
            entry->time.wMonth, entry->time.wDay, entry->time.wYear,
            entry->time.wHour, entry->time.wMinute, entry->time.wSecond);

        // Seek to the end of the log file
        SetFilePointer(logHandle, 0, 0, FILE_END);

        if (!WriteFile(logHandle, buffer, ELEMENT_COUNT(buffer) - remaining, &written, NULL)) {
        }
        WriteFile(logHandle, entry->message, entry->length, &written, NULL);

        MemFree(entry);
    }
}

static VOID FCALL QueueInsert(LOG_ENTRY *entry)
{
    BOOL empty;

    ASSERT(entry->length == strlen(entry->message));

    EnterCriticalSection(&logLock);

    empty = STAILQ_EMPTY(&logQueue);

    // Insert log entry at the queue's tail
    STAILQ_INSERT_TAIL(&logQueue, entry, link);

    if (empty) {
        // Since the queue was empty, have a worker thread call the writer
        Io_QueueJob(QueueWrite, NULL, JOB_PRIORITY_LOW);
    }

    LeaveCriticalSection(&logLock);
}


DWORD SCALL LogFileInit(VOID)
{
    CHAR    *path;
    DWORD   result;

    InterlockedExchange(&logStatus, LOG_STATUS_INACTIVE);
    STAILQ_INIT(&logQueue);

    path = Io_ConfigGetPath("Locations", "Log_Files", "nxMyDB.log", NULL);
    if (path == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Open log file for writing
    logHandle = CreateFileA(path, GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);

    if (logHandle == INVALID_HANDLE_VALUE) {
        result = GetLastError();

    } else if (!InitializeCriticalSectionAndSpinCount(&logLock, 50)) {
        result = GetLastError();
        CloseHandle(logHandle);

    } else {
        result = ERROR_SUCCESS;

        // Set logging system as active
        InterlockedExchange(&logStatus, LOG_STATUS_ACTIVE);

        // Write log header
        LogFileFormat(".-----------------------------------------.\r\n");
    }

    Io_Free(path);

    return result;
}

DWORD SCALL LogFileFinalize(VOID)
{
    // Write log footer
    LogFileFormat("'-----------------------------------------'\r\n");

    // Write all pending log entries
    InterlockedExchange(&logStatus, LOG_STATUS_SHUTDOWN);
    QueueWrite(NULL);
    InterlockedExchange(&logStatus, LOG_STATUS_INACTIVE);

    ASSERT(STAILQ_EMPTY(&logQueue));

    CloseHandle(logHandle);
    logHandle = INVALID_HANDLE_VALUE;

    DeleteCriticalSection(&logLock);

    return ERROR_SUCCESS;
}

VOID SCALL LogFileFormat(const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogFileFormatV(format, argList);
    va_end(argList);
}

VOID SCALL LogFileFormatV(const CHAR *format, va_list argList)
{
    DWORD       errorCode;
    LOG_ENTRY   *entry;
    size_t      remaining;

    // Preserve system error code
    errorCode = GetLastError();

    ASSERT(format != NULL);

    // Retrieve a spare entry structure (also checks log status)
    entry = GetSpareEntry();
    if (entry != NULL) {
        GetLocalTime(&entry->time);

        StringCchVPrintfExA(entry->message, ELEMENT_COUNT(entry->message),
            NULL, &remaining, 0, format, argList);

        entry->length = ELEMENT_COUNT(entry->message) - remaining;

        // Insert log entry into the write queue
        QueueInsert(entry);
    }

    // Restore system error code
    SetLastError(errorCode);
}

VOID SCALL LogFileTrace(const CHAR *file, const CHAR *func, INT line, const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogFileTraceV(file, func, line, format, argList);
    va_end(argList);
}

VOID SCALL LogFileTraceV(const CHAR *file, const CHAR *func, INT line, const CHAR *format, va_list argList)
{
    DWORD       errorCode;
    DWORD       processId;
    DWORD       threadId;
    LOG_ENTRY   *entry;
    size_t      remaining;

    // Preserve system error code
    errorCode = GetLastError();

    ASSERT(file != NULL);
    ASSERT(func != NULL);
    ASSERT(format != NULL);

    processId = GetCurrentProcessId();
    threadId  = GetCurrentThreadId();

    // Retrieve a spare entry structure (also checks log status)
    entry = GetSpareEntry();
    if (entry != NULL) {
        //
        // Available trace information:
        //
        // file      - Source file
        // func      - Function name in the source file
        // line      - Line number in the source file
        // processId - Current process ID
        // threadId  - Current thread ID
        //
        GetLocalTime(&entry->time);

        StringCchVPrintfExA(entry->message, ELEMENT_COUNT(entry->message),
            NULL, &remaining, 0, format, argList);

        entry->length = ELEMENT_COUNT(entry->message) - remaining;

        // Insert log entry into the write queue
        QueueInsert(entry);
    }

    // Restore system error code
    SetLastError(errorCode);
}
