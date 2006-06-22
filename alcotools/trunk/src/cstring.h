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

#ifndef _ALSOSTRING_H_
#define _ALSOSTRING_H_

// Max number of characters we support (same as INT_MAX).
#define STRSAFE_MAX_CCH     2147483647

// If both strsafe.h and ntstrsafe.h are included, only use definitions below from one.
#ifndef _NTSTRSAFE_H_INCLUDED_

// Flags for controling the Ex functions.
#define STRSAFE_IGNORE_NULLS                            0x00000100  // Treat null string pointers as TEXT("") -- don't fault on NULL buffers
#define STRSAFE_FILL_BEHIND_NULL                        0x00000200  // Fill in extra space behind the null terminator
#define STRSAFE_FILL_ON_FAILURE                         0x00000400  // On failure, overwrite pszDest with fill pattern and null terminate it
#define STRSAFE_NULL_ON_FAILURE                         0x00000800  // On failure, set *pszDest = TEXT('\0')
#define STRSAFE_NO_TRUNCATION                           0x00001000  // Instead of returning a truncated result, copy/append nothing to pszDest and null terminate it
#define STRSAFE_IGNORE_NULL_UNICODE_STRINGS             0x00010000  // Don't crash on null UNICODE_STRING pointers (STRSAFE_IGNORE_NULLS implies this flag)
#define STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED     0x00020000  // We will fail if the Dest PUNICODE_STRING's Buffer cannot be null terminated

#define STRSAFE_VALID_FLAGS                 (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)
#define STRSAFE_UNICODE_STRING_VALID_FLAGS  (STRSAFE_VALID_FLAGS | STRSAFE_IGNORE_NULL_UNICODE_STRINGS | STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED)

// Helper macro to set the fill character and specify buffer filling.
#define STRSAFE_FILL_BYTE(x)            ((unsigned long)((x & 0x000000FF) | STRSAFE_FILL_BEHIND_NULL))
#define STRSAFE_FAILURE_BYTE(x)         ((unsigned long)((x & 0x000000FF) | STRSAFE_FILL_ON_FAILURE))
#define STRSAFE_GET_FILL_PATTERN(x)     ((int)(x & 0x000000FF))

#endif // _NTSTRSAFE_H_INCLUDED_


int
StringCatA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    );

int
StringCatExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCatExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCatNA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToAppend
    );

int
StringCatNExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToAppend,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCatNExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToAppend,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCatNW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToAppend
    );

int
StringCatW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    );

int
StringCopyA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc
    );

int
StringCopyExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCopyExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCopyNA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToCopy
    );

int
StringCopyNExA(
    char *pszDest,
    size_t cchDest,
    const char *pszSrc,
    size_t cchToCopy,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCopyNExW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToCopy,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags
    );

int
StringCopyNW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc,
    size_t cchToCopy
    );

int
StringCopyW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszSrc
    );

int
StringLengthA(
    const char *psz,
    size_t cchMax,
    size_t *pcchLength
    );

int
StringLengthW(
    const WCHAR *psz,
    size_t cchMax,
    size_t *pcchLength
    );

int
StringPrintfA(
    char *pszDest,
    size_t cchDest,
    const char *pszFormat,
    ...
    );

int
StringPrintfExA(
    char *pszDest,
    size_t cchDest,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const char *pszFormat,
    ...
    );

int
StringPrintfExW(
    WCHAR *pszDest,
    size_t cchDest,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const WCHAR *pszFormat,
    ...
    );

int
StringPrintfW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszFormat,
    ...
    );

int
StringVPrintfA(
    char *pszDest,
    size_t cchDest,
    const char *pszFormat,
    va_list argList
    );

int
StringVPrintfExA(
    char *pszDest,
    size_t cchDest,
    char **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const char *pszFormat,
    va_list argList
    );

int
StringVPrintfExW(
    WCHAR *pszDest,
    size_t cchDest,
    WCHAR **ppszDestEnd,
    size_t *pcchRemaining,
    unsigned long dwFlags,
    const WCHAR *pszFormat,
    va_list argList
    );

int
StringVPrintfW(
    WCHAR *pszDest,
    size_t cchDest,
    const WCHAR *pszFormat,
    va_list argList
    );

#ifdef UNICODE
#   define StringCat         StringCatW
#   define StringCatEx       StringCatExW
#   define StringCatN        StringCatNW
#   define StringCatNEx      StringCatNExW
#   define StringCopy        StringCopyW
#   define StringCopyEx      StringCopyExW
#   define StringCopyN       StringCopyNW
#   define StringCopyNEx     StringCopyNExW
#   define StringLength      StringLengthW
#   define StringPrintf      StringPrintfW
#   define StringPrintfEx    StringPrintfExW
#   define StringVPrintf     StringVPrintfW
#   define StringVPrintfEx   StringVPrintfExW
#else
#   define StringCat         StringCatA
#   define StringCatEx       StringCatExA
#   define StringCatN        StringCatNA
#   define StringCatNEx      StringCatNExA
#   define StringCopy        StringCopyA
#   define StringCopyEx      StringCopyExA
#   define StringCopyN       StringCopyNA
#   define StringCopyNEx     StringCopyNExA
#   define StringLength      StringLengthA
#   define StringPrintf      StringPrintfA
#   define StringPrintfEx    StringPrintfExA
#   define StringVPrintf     StringVPrintfA
#   define StringVPrintfEx   StringVPrintfExA
#endif // UNICODE

#endif // _ALSOSTRING_H_
