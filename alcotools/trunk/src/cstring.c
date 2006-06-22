/*++

Safe String Functions
Copyright (c) Microsoft Corp. All rights reserved.

Module Name:
    C String

Author:
    Microsoft

Abstract:
    This module defines safer C library string routine replacements. These are
    meant to make C a bit more safe in reference to security and robustness.

    For more information, see:
    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnsecure/html/strsafe.asp

--*/

#include "alcoholicz.h"

#ifndef USE_SECURE_CRT
#   if defined(__GOT_SECURE_LIB__) && (__GOT_SECURE_LIB__ >= 200402L)
#       define USE_SECURE_CRT 1
#   else
#       define USE_SECURE_CRT 0
#   endif
#endif

static int
StringCopyWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    );

static int
StringCopyWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    );

static int
StringCopyExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringCopyExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringCopyNWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToCopy
    );

static int
StringCopyNWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToCopy
    );

static int
StringCopyNExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    size_t cchToCopy,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringCopyNExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    size_t cchToCopy,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringCatWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    );

static int
StringCatWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    );

static int
StringCatExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringCatExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringCatNWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToAppend
    );

static int
StringCatNWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToAppend
    );

static int
StringCatNExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    size_t cchToAppend,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringCatNExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    size_t cchToAppend,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

static int
StringVPrintfWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszFormat,
    va_list argList
    );

static int
StringVPrintfWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszFormat,
    va_list argList
    );

static int
StringVPrintfExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const char *pszFormat,
    va_list argList
    );

static int
StringVPrintfExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const WCHAR *pszFormat,
    va_list argList
    );

static int
StringLengthWorkerA(
    const char *psz,
    size_t cchMax,
    size_t *pcchLength
    );

static int
StringLengthWorkerW(
    const WCHAR *psz,
    size_t cchMax,
    size_t *pcchLength
    );


/*++

StringCopy

    This routine is a safer version of the C built-in function 'strcpy'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This routine is not a replacement for strncpy.  That function will pad the
    destination string with extra null termination characters if the count is
    greater than the length of the source string, and it will fail to null
    terminate the destination string if the source string length is greater
    than or equal to the count. You can not blindly use this instead of strncpy:
    it is common for code to use it to "patch" strings and you would introduce
    errors if the code started null terminating in the middle of the string.

    This function returns a hresult, and not a pointer.  It returns
    ALCOHOL_OK if the string was copied without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of
    pszSrc will be copied to pszDest as possible, and pszDest will be null
    terminated.

Arguments:
    pszDest        -   destination string

    cchDest        -   size of destination buffer in characters.
                       length must be = (_tcslen(src) + 1) to hold all of the
                       source including the null terminator

    pszSrc         -   source string which must be null terminated

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See StringCopyEx if you require
    the handling of NULL values.

--*/

int
StringCopyA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCopyWorkerA(pszDest, cchDest, pszSrc);
    }

    return result;
}

int
StringCopyW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCopyWorkerW(pszDest, cchDest, pszSrc);
    }

    return result;
}

/*++

StringCopyEx

    This routine is a safer version of the C built-in function 'strcpy' with
    some additional parameters.  In addition to functionality provided by
    StringCopy, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:
    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = (_tcslen(pszSrc) + 1) to hold all of
                        the source including the null terminator

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    ALCOHOL_INSUFFICIENT_BUFFER

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is ALCOHOL_INSUFFICIENT_BUFFER.

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

--*/

int
StringCopyExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        result = StringCopyExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

int
StringCopyExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(WCHAR) since cchDest < STRSAFE_MAX_CCH and sizeof(WCHAR) is 2
        cbDest = cchDest * sizeof(WCHAR);

        result = StringCopyExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

/*++

StringCopyN

    This routine is a safer version of the C built-in function 'strncpy'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cchToCopy is greater than the length of pszSrc.

    This function returns a hresult, and not a pointer.  It returns
    ALCOHOL_OK if the entire string or the first cchToCopy characters were copied
    without truncation and the resultant destination string was null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be copied to pszDest as possible, and pszDest will be null terminated.

Arguments:
    pszDest        -   destination string

    cchDest        -   size of destination buffer in characters.
                       length must be = (_tcslen(src) + 1) to hold all of the
                       source including the null terminator

    pszSrc         -   source string

    cchToCopy      -   maximum number of characters to copy from source string,
                       not including the null terminator.

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See StringCopyNEx if you require
    the handling of NULL values.

--*/

int
StringCopyNA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToCopy
    )
{
    int result;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchToCopy > STRSAFE_MAX_CCH))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCopyNWorkerA(pszDest, cchDest, pszSrc, cchToCopy);
    }

    return result;
}

int
StringCopyNW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToCopy
    )
{
    int result;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchToCopy > STRSAFE_MAX_CCH))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCopyNWorkerW(pszDest, cchDest, pszSrc, cchToCopy);
    }

    return result;
}

/*++

StringCopyNEx

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters.  In addition to functionality provided by
    StringCopyN, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination
    string including the null terminator. The flags parameter allows
    additional controls.

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cchToCopy is greater than the length of pszSrc.

Arguments:
    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = (_tcslen(pszSrc) + 1) to hold all of
                        the source including the null terminator

    pszSrc          -   source string

    cchToCopy       -   maximum number of characters to copy from the source
                        string

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    ALCOHOL_INSUFFICIENT_BUFFER

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is ALCOHOL_INSUFFICIENT_BUFFER.

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified. If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL. An error may still be returned even though NULLS are ignored
    due to insufficient space.

--*/

int
StringCopyNExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToCopy,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        result = StringCopyNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchToCopy, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

int
StringCopyNExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToCopy,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(WCHAR) since cchDest < STRSAFE_MAX_CCH and sizeof(WCHAR) is 2
        cbDest = cchDest * sizeof(WCHAR);

        result = StringCopyNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchToCopy, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

/*++

StringCat

    This routine is a safer version of the C built-in function 'strcat'.
    The size of the destination buffer (in characters) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This function returns a hresult, and not a pointer.  It returns
    ALCOHOL_OK if the string was concatenated without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be appended to pszDest as possible, and pszDest will be null
    terminated.

Arguments:
    pszDest     -  destination string which must be null terminated

    cchDest     -  size of destination buffer in characters.
                   length must be = (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
                   to hold all of the combine string plus the null
                   terminator

    pszSrc      -  source string which must be null terminated

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See StringCatEx if you require
    the handling of NULL values.

--*/

int
StringCatA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCatWorkerA(pszDest, cchDest, pszSrc);
    }

    return result;
}

int
StringCatW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCatWorkerW(pszDest, cchDest, pszSrc);
    }

    return result;
}

/*++

StringCatEx

    This routine is a safer version of the C built-in function 'strcat' with
    some additional parameters.  In addition to functionality provided by
    StringCat, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:
    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters
                        length must be (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcat

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns ALCOHOL_INSUFFICIENT_BUFFER, pszDest
                    will not contain a truncated string, it will remain unchanged.

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

--*/

int
StringCatExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        result = StringCatExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

int
StringCatExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(WCHAR) since cchDest < STRSAFE_MAX_CCH and sizeof(WCHAR) is 2
        cbDest = cchDest * sizeof(WCHAR);

        result = StringCatExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

/*++

StringCatN

    This routine is a safer version of the C built-in function 'strncat'.
    The size of the destination buffer (in characters) is a parameter as well as
    the maximum number of characters to append, excluding the null terminator.
    This function will not write past the end of the destination buffer and it will
    ALWAYS null terminate pszDest (unless it is zero length).

    This function returns a hresult, and not a pointer.  It returns
    ALCOHOL_OK if all of pszSrc or the first cchToAppend characters were appended
    to the destination string and it was null terminated, otherwise it will
    return a failure code. In failure cases as much of pszSrc will be appended
    to pszDest as possible, and pszDest will be null terminated.

Arguments:
    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters.
                        length must be (_tcslen(pszDest) + min(cchToAppend, _tcslen(pszSrc)) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cchToAppend     -   maximum number of characters to append

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See StringCatNEx if you require
    the handling of NULL values.

--*/

int
StringCatNA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToAppend
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCatNWorkerA(pszDest, cchDest, pszSrc, cchToAppend);
    }

    return result;
}

int
StringCatNW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToAppend
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringCatNWorkerW(pszDest, cchDest, pszSrc, cchToAppend);
    }

    return result;
}

/*++

StringCatNEx

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters.  In addition to functionality provided by
    StringCatN, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:
    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters.
                        length must be (_tcslen(pszDest) + min(cchToAppend, _tcslen(pszSrc)) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cchToAppend     -   maximum number of characters to append

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns ALCOHOL_INSUFFICIENT_BUFFER, pszDest
                    will not contain a truncated string, it will remain unchanged.

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

--*/

int
StringCatNExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToAppend,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        result = StringCatNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchToAppend, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

int
StringCatNExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToAppend,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(WCHAR) since cchDest < STRSAFE_MAX_CCH and sizeof(WCHAR) is 2
        cbDest = cchDest * sizeof(WCHAR);

        result = StringCatNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchToAppend, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return result;
}

/*++

StringVPrintf

    This routine is a safer version of the C built-in function 'vsprintf'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns a hresult, and not a pointer.  It returns
    ALCOHOL_OK if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:
    pszDest     -  destination string

    cchDest     -  size of destination buffer in characters
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    argList     -  va_list from the variable arguments according to the
                   stdarg.h convention

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See StringVPrintfEx if you
    require the handling of NULL values.

--*/

int
StringVPrintfA(
    char *pszDest,
    size_t cchDest,
    const char *pszFormat,
    va_list argList
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);
    }

    return result;
}

int
StringVPrintfW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszFormat,
    va_list argList
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);
    }

    return result;
}

/*++

StringPrintf

    This routine is a safer version of the C built-in function 'sprintf'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns a hresult, and not a pointer.  It returns
    ALCOHOL_OK if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:
    pszDest     -  destination string

    cchDest     -  size of destination buffer in characters
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    ...         -  additional parameters to be formatted according to
                   the format string

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See StringPrintfEx if you
    require the handling of NULL values.

--*/

int
StringPrintfA(
    char *pszDest,
    size_t cchDest,
    const char *pszFormat,
    ...
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        result = StringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);

        va_end(argList);
    }

    return result;
}

int
StringPrintfW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszFormat,
    ...
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        result = StringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);

        va_end(argList);
    }

    return result;
}

/*++

StringPrintfEx

    This routine is a safer version of the C built-in function 'sprintf' with
    some additional parameters.  In addition to functionality provided by
    StringPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:
    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return
                        the number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    ALCOHOL_INSUFFICIENT_BUFFER

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is ALCOHOL_INSUFFICIENT_BUFFER.

    pszFormat       -   format string which must be null terminated

    ...             -   additional parameters to be formatted according to
                        the format string

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

--*/

int
StringPrintfExA(
    char *pszDest,
    size_t cchDest,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const char *pszFormat,
    ...
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;
        va_list argList;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);
        va_start(argList, pszFormat);

        result = StringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);

        va_end(argList);
    }

    return result;
}

int
StringPrintfExW(
    WCHAR *pszDest,
    size_t cchDest,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const WCHAR *pszFormat,
    ...
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;
        va_list argList;

        // safe to multiply cchDest * sizeof(WCHAR) since cchDest < STRSAFE_MAX_CCH and sizeof(WCHAR) is 2
        cbDest = cchDest * sizeof(WCHAR);
        va_start(argList, pszFormat);

        result = StringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);

        va_end(argList);
    }

    return result;
}

/*++

StringVPrintfEx

    This routine is a safer version of the C built-in function 'vsprintf' with
    some additional parameters.  In addition to functionality provided by
    StringVPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:
    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return
                        the number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    ALCOHOL_INSUFFICIENT_BUFFER

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is ALCOHOL_INSUFFICIENT_BUFFER.

    pszFormat       -   format string which must be null terminated

    argList         -   va_list from the variable arguments according to the
                        stdarg.h convention

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

--*/

int
StringVPrintfExA(
    char *pszDest,
    size_t cchDest,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const char *pszFormat,
    va_list argList
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        result = StringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
    }

    return result;
}

int
StringVPrintfExW(
    WCHAR *pszDest,
    size_t cchDest,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const WCHAR *pszFormat,
    va_list argList
    )
{
    int result;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(WCHAR) since cchDest < STRSAFE_MAX_CCH and sizeof(WCHAR) is 2
        cbDest = cchDest * sizeof(WCHAR);

        result = StringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
    }

    return result;
}

/*++

StringLength

    This routine is a safer version of the C built-in function 'strlen'.
    It is used to make sure a string is not larger than a given length, and
    it optionally returns the current length in characters not including
    the null terminator.

    This function returns a hresult, and not a pointer.  It returns
    ALCOHOL_OK if the string is non-null and the length including the null
    terminator is less than or equal to cchMax characters.

Arguments:
    psz         -   string to check the length of

    cchMax      -   maximum number of characters including the null terminator
                    that psz is allowed to contain

    pcch        -   if the function succeeds and pcch is non-null, the current length
                    in characters of psz excluding the null terminator will be returned.
                    This out parameter is equivalent to the return value of strlen(psz)

Return Value:
    If the function succeeds, the return value is ALCOHOL_OK. If the
    function fails, the return value is an appropriate error code.

Remarks:
    psz can be null but the function will fail

    cchMax should be greater than zero or the function will fail

--*/

int
StringLengthA(
    const char *psz,
    size_t cchMax,
    size_t *pcchLength
    )
{
    int result;

    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringLengthWorkerA(psz, cchMax, pcchLength);
    }

    if (result != ALCOHOL_OK && pcchLength)
    {
        *pcchLength = 0;
    }

    return result;
}

int
StringLengthW(
    const WCHAR *psz,
    size_t cchMax,
    size_t *pcchLength
    )
{
    int result;

    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        result = StringLengthWorkerW(psz, cchMax, pcchLength);
    }

    if (result != ALCOHOL_OK && pcchLength)
    {
        *pcchLength = 0;
    }

    return result;
}


// Below here are the worker functions that actually do the work
int
StringCopyWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    )
{
    int result = ALCOHOL_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && (*pszSrc != '\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            result = ALCOHOL_INSUFFICIENT_BUFFER;
        }

        *pszDest= '\0';
    }

    return result;
}

int
StringCopyWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    )
{
    int result = ALCOHOL_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && (*pszSrc != L'\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            result = ALCOHOL_INSUFFICIENT_BUFFER;
        }

        *pszDest= L'\0';
    }

    return result;
}

int
StringCopyExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    char *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && (*pszSrc != '\0'))
                {
                    *pszDestEnd++ = *pszSrc++;
                    cchRemaining--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    result = ALCOHOL_INSUFFICIENT_BUFFER;
                }

                *pszDestEnd = '\0';
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringCopyExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    WCHAR *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(WCHAR)) ||
    //        cbDest == (cchDest * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && (*pszSrc != L'\0'))
                {
                    *pszDestEnd++ = *pszSrc++;
                    cchRemaining--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    result = ALCOHOL_INSUFFICIENT_BUFFER;
                }

                *pszDestEnd = L'\0';
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringCopyNWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchSrc
    )
{
    int result = ALCOHOL_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && cchSrc && (*pszSrc != '\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
            cchSrc--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            result = ALCOHOL_INSUFFICIENT_BUFFER;
        }

        *pszDest= '\0';
    }

    return result;
}

int
StringCopyNWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToCopy
    )
{
    int result = ALCOHOL_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && cchToCopy && (*pszSrc != L'\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
            cchToCopy--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            result = ALCOHOL_INSUFFICIENT_BUFFER;
        }

        *pszDest= L'\0';
    }

    return result;
}

int
StringCopyNExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    size_t cchToCopy,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    char *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else if (cchToCopy > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && cchToCopy && (*pszSrc != '\0'))
                {
                    *pszDestEnd++ = *pszSrc++;
                    cchRemaining--;
                    cchToCopy--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    result = ALCOHOL_INSUFFICIENT_BUFFER;
                }

                *pszDestEnd = '\0';
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringCopyNExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    size_t cchToCopy,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    WCHAR *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(WCHAR)) ||
    //        cbDest == (cchDest * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else if (cchToCopy > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && cchToCopy && (*pszSrc != L'\0'))
                {
                    *pszDestEnd++ = *pszSrc++;
                    cchRemaining--;
                    cchToCopy--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    result = ALCOHOL_INSUFFICIENT_BUFFER;
                }

                *pszDestEnd = L'\0';
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringCatWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    )
{
    int result;
    size_t cchDestLength;

    result = StringLengthWorkerA(pszDest, cchDest, &cchDestLength);

    if (result == ALCOHOL_OK)
    {
        result = StringCopyWorkerA(pszDest + cchDestLength,
                               cchDest - cchDestLength,
                               pszSrc);
    }

    return result;
}

int
StringCatWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    )
{
    int result;
    size_t cchDestLength;

    result = StringLengthWorkerW(pszDest, cchDest, &cchDestLength);

    if (result == ALCOHOL_OK)
    {
        result = StringCopyWorkerW(pszDest + cchDestLength,
                               cchDest - cchDestLength,
                               pszSrc);
    }

    return result;
}

int
StringCatExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    char *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cchDestLength;

        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestLength = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }
            else
            {
                result = StringLengthWorkerA(pszDest, cchDest, &cchDestLength);

                if (result == ALCOHOL_OK)
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }
        else
        {
            result = StringLengthWorkerA(pszDest, cchDest, &cchDestLength);

            if (result == ALCOHOL_OK)
            {
                pszDestEnd = pszDest + cchDestLength;
                cchRemaining = cchDest - cchDestLength;
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                result = StringCopyExWorkerA(pszDestEnd,
                                         cchRemaining,
                                         (cchRemaining * sizeof(char)) + (cbDest % sizeof(char)),
                                         pszSrc,
                                         &pszDestEnd,
                                         &cchRemaining,
                                         dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by StringCopyExWorkerA()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringCatExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    WCHAR *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(WCHAR)) ||
    //        cbDest == (cchDest * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        size_t cchDestLength;

        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestLength = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }
            else
            {
                result = StringLengthWorkerW(pszDest, cchDest, &cchDestLength);

                if (result == ALCOHOL_OK)
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }
        else
        {
            result = StringLengthWorkerW(pszDest, cchDest, &cchDestLength);

            if (result == ALCOHOL_OK)
            {
                pszDestEnd = pszDest + cchDestLength;
                cchRemaining = cchDest - cchDestLength;
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                result = StringCopyExWorkerW(pszDestEnd,
                                         cchRemaining,
                                         (cchRemaining * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)),
                                         pszSrc,
                                         &pszDestEnd,
                                         &cchRemaining,
                                         dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by StringCopyExWorkerW()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringCatNWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToAppend
    )
{
    int result;
    size_t cchDestLength;

    result = StringLengthWorkerA(pszDest, cchDest, &cchDestLength);

    if (result == ALCOHOL_OK)
    {
        result = StringCopyNWorkerA(pszDest + cchDestLength,
                                cchDest - cchDestLength,
                                pszSrc,
                                cchToAppend);
    }

    return result;
}

int
StringCatNWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToAppend
    )
{
    int result;
    size_t cchDestLength;

    result = StringLengthWorkerW(pszDest, cchDest, &cchDestLength);

    if (result == ALCOHOL_OK)
    {
        result = StringCopyNWorkerW(pszDest + cchDestLength,
                                cchDest - cchDestLength,
                                pszSrc,
                                cchToAppend);
    }

    return result;
}

int
StringCatNExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    const char *pszSrc,
    size_t cchToAppend,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    char *pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestLength = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else if (cchToAppend > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestLength = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }
            else
            {
                result = StringLengthWorkerA(pszDest, cchDest, &cchDestLength);

                if (result == ALCOHOL_OK)
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }
        else
        {
            result = StringLengthWorkerA(pszDest, cchDest, &cchDestLength);

            if (result == ALCOHOL_OK)
            {
                pszDestEnd = pszDest + cchDestLength;
                cchRemaining = cchDest - cchDestLength;
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                result = StringCopyNExWorkerA(pszDestEnd,
                                          cchRemaining,
                                          (cchRemaining * sizeof(char)) + (cbDest % sizeof(char)),
                                          pszSrc,
                                          cchToAppend,
                                          &pszDestEnd,
                                          &cchRemaining,
                                          dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by StringCopyNExWorkerA()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringCatNExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    const WCHAR *pszSrc,
    size_t cchToAppend,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    )
{
    int result = ALCOHOL_OK;
    WCHAR *pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestLength = 0;


    // ASSERT(cbDest == (cchDest * sizeof(WCHAR)) ||
    //        cbDest == (cchDest * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else if (cchToAppend > STRSAFE_MAX_CCH)
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestLength = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }
            else
            {
                result = StringLengthWorkerW(pszDest, cchDest, &cchDestLength);

                if (result == ALCOHOL_OK)
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }
        else
        {
            result = StringLengthWorkerW(pszDest, cchDest, &cchDestLength);

            if (result == ALCOHOL_OK)
            {
                pszDestEnd = pszDest + cchDestLength;
                cchRemaining = cchDest - cchDestLength;
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                result = StringCopyNExWorkerW(pszDestEnd,
                                          cchRemaining,
                                          (cchRemaining * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)),
                                          pszSrc,
                                          cchToAppend,
                                          &pszDestEnd,
                                          &cchRemaining,
                                          dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by StringCopyNExWorkerW()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringVPrintfWorkerA(
    char *pszDest,
    size_t cchDest,
    const char *pszFormat,
    va_list argList
    )
{
    int result = ALCOHOL_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        int iRet;
        size_t cchMax;

        // leave the last space for the null terminator
        cchMax = cchDest - 1;

#if (USE_SECURE_CRT == 1)
        iRet = _vsnprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
        iRet = vsnprintf(pszDest, cchMax, pszFormat, argList);
#endif
        // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = '\0';

            // we have truncated pszDest
            result = ALCOHOL_INSUFFICIENT_BUFFER;
        }
        else if (((size_t)iRet) == cchMax)
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = '\0';
        }
    }

    return result;
}

int
StringVPrintfWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszFormat,
    va_list argList
    )
{
    int result = ALCOHOL_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        int iRet;
        size_t cchMax;

        // leave the last space for the null terminator
        cchMax = cchDest - 1;

#if (USE_SECURE_CRT == 1)
        iRet = _vsnwprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
        iRet = vsnwprintf(pszDest, cchMax, pszFormat, argList);
#endif
        // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = L'\0';

            // we have truncated pszDest
            result = ALCOHOL_INSUFFICIENT_BUFFER;
        }
        else if (((size_t)iRet) == cchMax)
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = L'\0';
        }
    }

    return result;
}

int
StringVPrintfExWorkerA(
    char *pszDest,
    size_t cchDest,
    size_t cbDest,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const char *pszFormat,
    va_list argList
    )
{
    int result = ALCOHOL_OK;
    char *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }

            if (pszFormat == NULL)
            {
                pszFormat = "";
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                int iRet;
                size_t cchMax;

                // leave the last space for the null terminator
                cchMax = cchDest - 1;

#if (USE_SECURE_CRT == 1)
                iRet = _vsnprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
                iRet = vsnprintf(pszDest, cchMax, pszFormat, argList);
#endif
                // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    // we have truncated pszDest
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = '\0';

                    result = ALCOHOL_INSUFFICIENT_BUFFER;
                }
                else if (((size_t)iRet) == cchMax)
                {
                    // string fit perfectly
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = '\0';
                }
                else if (((size_t)iRet) < cchMax)
                {
                    // there is extra room
                    pszDestEnd = pszDest + iRet;
                    cchRemaining = cchDest - iRet;

                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                    }
                }
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringVPrintfExWorkerW(
    WCHAR *pszDest,
    size_t cchDest,
    size_t cbDest,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const WCHAR *pszFormat,
    va_list argList
    )
{
    int result = ALCOHOL_OK;
    WCHAR *pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(WCHAR)) ||
    //        cbDest == (cchDest * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        result = ALCOHOL_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    result = ALCOHOL_INVALID_PARAMETER;
                }
            }

            if (pszFormat == NULL)
            {
                pszFormat = L"";
            }
        }

        if (result == ALCOHOL_OK)
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        result = ALCOHOL_INVALID_PARAMETER;
                    }
                    else
                    {
                        result = ALCOHOL_INSUFFICIENT_BUFFER;
                    }
                }
            }
            else
            {
                int iRet;
                size_t cchMax;

                // leave the last space for the null terminator
                cchMax = cchDest - 1;

#if (USE_SECURE_CRT == 1)
                iRet = _vsnwprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
                iRet = vsnwprintf(pszDest, cchMax, pszFormat, argList);
#endif
                // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    // we have truncated pszDest
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = L'\0';

                    result = ALCOHOL_INSUFFICIENT_BUFFER;
                }
                else if (((size_t)iRet) == cchMax)
                {
                    // string fit perfectly
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = L'\0';
                }
                else if (((size_t)iRet) < cchMax)
                {
                    // there is extra room
                    pszDestEnd = pszDest + iRet;
                    cchRemaining = cchDest - iRet;

                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(WCHAR)) + (cbDest % sizeof(WCHAR)));
                    }
                }
            }
        }
    }

    if (result != ALCOHOL_OK)
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if ((result == ALCOHOL_OK) || (result == ALCOHOL_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return result;
}

int
StringLengthWorkerA(
    const char *psz,
    size_t cchMax,
    size_t *pcchLength
    )
{
    int result = ALCOHOL_OK;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != '\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        result = ALCOHOL_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (result == ALCOHOL_OK)
        {
            *pcchLength = cchMaxPrev - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return result;
}

int
StringLengthWorkerW(
    const WCHAR *psz,
    size_t cchMax,
    size_t *pcchLength
    )
{
    int result = ALCOHOL_OK;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        result = ALCOHOL_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (result == ALCOHOL_OK)
        {
            *pcchLength = cchMaxPrev - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return result;
}
