#include <Windows.h>
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

void __stdcall RecursiveCleaner(LPCTSTR lpszCheckPath)
{
	DWORD dwRetVal;
	TCHAR szOriginalPath[MAX_PATH];

	if ((dwRetVal = GetCurrentDirectory(MAX_PATH, szOriginalPath)) == 0 || dwRetVal > MAX_PATH) {
		OutputMessage(_T(" - Unable to retrieve current directory (error %lu).\n"), GetLastError());
	} else if (!SetCurrentDirectory(lpszCheckPath)) {
		OutputMessage(_T(" - Unable to change directory: \"%s\" (error %lu).\n"), lpszCheckPath, GetLastError());
	} else {
		HANDLE hFind;
		WIN32_FIND_DATA lpFindFileData;

#ifdef TEST_MODE
		TCHAR szCurrentDir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, szCurrentDir);

		OutputMessage(_T("Test: Checking: %s\n"), szCurrentDir);
#endif

		hFind = FindFirstFile(_T("*"), &lpFindFileData);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (!_tcscmp(lpFindFileData.cFileName, _T(".")) ||
					!_tcscmp(lpFindFileData.cFileName, _T("..")) ||
					lpFindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
					continue;
				}
				if (lpFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					RecursiveCleaner(lpFindFileData.cFileName);
				} else if (_tcsicmp(lpFindFileData.cFileName, _T(".ioFTPD")) == 0) {
#ifdef TEST_MODE
					OutputMessage(_T("Test: Found .ioFTPD file (%lu bytes).\n"), lpFindFileData.nFileSizeLow);
#endif
					// Remove .ioFTPD files >= then IOFILE_MAX_SIZE (4096) bytes
					if (lpFindFileData.nFileSizeLow >= IOFILE_MAX_SIZE) {
#ifndef TEST_MODE
						if (DeleteFile(lpFindFileData.cFileName)) {
							OutputMessage(_T(" - Removed .ioFTPD file (%lu bytes).\n"), lpFindFileData.nFileSizeLow);
						} else {
							OutputMessage(_T(" - Unable to remove .ioFTPD file (%lu bytes, error %lu).\n"), lpFindFileData.nFileSizeLow, GetLastError());
						}
#else
						OutputMessage(_T("Test: Removed .ioFTPD file (%lu bytes).\n"), lpFindFileData.nFileSizeLow);
#endif
					}
				}
			} while (FindNextFile(hFind, &lpFindFileData) != FALSE);

			FindClose(hFind);
		}
		SetCurrentDirectory(szOriginalPath);
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