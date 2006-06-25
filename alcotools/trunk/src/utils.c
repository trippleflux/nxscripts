/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements miscellaneous utilities.

--*/

#include "alcoholicz.h"

/*++

GetErrorMessage

    Returns a string explaining the APR status code.

Arguments:
    status  - APR status code.

Return Values:
    Pointer to a statically allocated buffer containing a human-readable
    message explaining the APR status code. The contents of this buffer
    must not be modified.

Remarks:
    This function is not thread-safe.

--*/
const char *
GetErrorMessage(
    apr_status_t status
    )
{
    static char message[512]; // THREADING: Static variable not thread-safe.
    return apr_strerror(status, message, ARRAYSIZE(message));
}
