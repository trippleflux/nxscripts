// System includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#pragma comment (lib,"shlwapi.lib")

// Standard includes
#include <StdIo.h>
#include <StdLib.h>
#include <StdArg.h>
#include <TChar.h>

// Maximum size of .ioFTPD allowed
#define IOFILE_MAX_SIZE 4096

// Test mode only (no deletion)
//#define TEST_MODE

BOOL bSilent = FALSE;

void __cdecl OutputMessage(LPCTSTR lpFormat, ...)
{
	if (!bSilent) {
		va_list pArgs;
		va_start(pArgs, lpFormat);
		_vtprintf(lpFormat, pArgs);
		fflush(stdout);
	}
}

void __stdcall RecursiveCleaner(LPCTSTR pszCurrentPath)
{
	DWORD dwRetVal;
	TCHAR szPath[MAX_PATH];

	if (!PathCombine(szPath, pszCurrentPath, _T("*.*"))) {
		OutputMessage(_T(" - Unable to combine path \"%s\" (error %lu).\n"), pszCurrentPath, GetLastError());
	} else {
		HANDLE hFind;
		WIN32_FIND_DATA pFindData;

#ifdef TEST_MODE
		OutputMessage(_T("Test: Checking: %s\n"), szPath);
#endif

		hFind = FindFirstFile(szPath, &pFindData);
		if (hFind == INVALID_HANDLE_VALUE) {

			do {
				if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ||
					!_tcscmp(pFindData.cFileName, _T(".")) ||
					!_tcscmp(pFindData.cFileName, _T("..")) ||
					!PathCombine(szPath, pszCurrentPath, pFindData.cFileName)) {
					continue;
				}

				if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					RecursiveCleaner(szPath);
				} else if (!_tcsicmp(pFindData.cFileName, _T(".ioFTPD"))) {
#ifdef TEST_MODE
					OutputMessage(_T("Test: Found .ioFTPD file (%lu bytes).\n"), pFindData.nFileSizeLow);
#endif
					if (pFindData.nFileSizeLow >= IOFILE_MAX_SIZE) {
#ifndef TEST_MODE
						if (DeleteFile(szPath)) {
							OutputMessage(_T(" - Removed .ioFTPD file (%lu bytes).\n"), pFindData.nFileSizeLow);
						} else {
							OutputMessage(_T(" - Unable to remove .ioFTPD file (%lu bytes, error %lu).\n"), pFindData.nFileSizeLow, GetLastError());
						}
#else
						OutputMessage(_T("Test: Removed .ioFTPD file (%lu bytes).\n"), pFindData.nFileSizeLow);
#endif
					}
				}
			} while (FindNextFile(hFind, &pFindData));

			FindClose(hFind);
		}
	}
}

INT __cdecl _tmain(INT argc, LPTSTR argv[])
{
	INT iReturn = 0;

	if (argc < 2) {
		if (_tgetenv(_T("USER")) != NULL && _tgetenv(_T("GROUP")) != NULL)
			_tprintf(_T("Syntax: SITE IOFILECHECK <phsyical path> [<phsyical path> <phsyical path> ...]\n"));
		else
			_tprintf(_T("Syntax: %s <phsyical path> [<phsyical path> <phsyical path> ...]\n"), argv[0]);
		iReturn = -1;
	} else {
		BOOL bUploadParam = FALSE;
		INT i;

		// Turn ioFTPD's buffer off
		_tprintf(_T("!buffer off\n"));
		fflush(stdout);

		for (i = 1; i < argc; i++) {
			// Special parameters
			if (!_tcscmp(argv[i], _T("-detach"))) {
				bSilent = TRUE;
				_tprintf(_T("!detach 0\n"));
				fflush(stdout);
			} else if (!_tcscmp(argv[i], _T("-silent"))) {
				bSilent = TRUE;
			} else if (!_tcscmp(argv[i], _T("-upload"))) {
				bUploadParam = TRUE;
			}
		}

		if (bUploadParam) {
			if (_tgetenv(_T("SYSTEMPATH")) == NULL) {
				_tprintf(_T("Missing ioftpd.env environment variable: SYSTEMPATH=%[$path]\n"));
				iReturn = -1;
			} else {
				bSilent = TRUE;
				RecursiveCleaner(_tgetenv(_T("SYSTEMPATH")));
			}
		} else {
			for (i = 1; i < argc; i++) {
				// Ignore any parameters
				if (_tcscmp(argv[i], _T("-detach")) != 0 ||
					_tcscmp(argv[i], _T("-silent")) != 0 ||
					_tcscmp(argv[i], _T("-upload")) != 0) {
					OutputMessage(_T(" Checking: %s\n"), argv[i]);
					RecursiveCleaner(argv[i]);
				}
			}
		}
	}
	return iReturn;
}