//	Window messages
#define WM_KICK				(WM_USER + 6)
#define WM_KILL				(WM_USER + 15)
#define WM_PHANDLE			(WM_USER + 17)
#define	WM_SHMEM			(WM_USER + 101)
#define WM_PID                          (WM_USER + 18)
#define	WM_DATACOPY_SHELLALLOC		(WM_USER + 19)
#define	WM_DATACOPY_FREE		(WM_USER + 20)
#define WM_DATACOPY_FILEMAP		(WM_USER + 21)
#define WM_ACCEPTEX			(WM_USER + 22)



//	Status flags
#define	S_DEAD		0010


typedef struct _ONLINEDATA
{
	INT32		Uid;

	DWORD		dwFlags;
	TCHAR		tszServiceName[_MAX_NAME + 1];	// Name of service
	TCHAR		tszAction[64];					// User's last action

	ULONG		ulClientIp;
	USHORT		usClientPort;

	CHAR		szHostName[MAX_HOSTNAME];	// Hostname
	CHAR		szIdent[MAX_IDENT];		// Ident

	TCHAR		tszVirtualPath[_MAX_PWD + 1];	// Virtual path
	LPTSTR		tszRealPath;					// Real path
	DWORD		dwRealPath;

	time_t		tLoginTime;				// Login Time
	DWORD		dwIdleTickCount;		// Idle Time

	BYTE		bTransferStatus;		// (0 Inactive, 1 Upload, 2 Download, 3 List)
	ULONG		ulDataClientIp;
	USHORT		usDataClientPort;

	TCHAR		tszVirtualDataPath[_MAX_PWD + 1];
	LPTSTR		tszRealDataPath;
	DWORD		dwRealDataPath;

	DWORD		dwBytesTransfered;		// Bytes transfered during interval
	DWORD		dwIntervalLength;		// Milliseconds
	INT64		i64TotalBytesTransfered;	// Total bytes transfered during transfer

} ONLINEDATA, * PONLINEDATA;


HWND GetMainWindow(VOID);
BOOL InstallMessageHandler(DWORD dwMessage, LPVOID lpProc, BOOL bInstantOperation);