typedef struct _GROUPFILE
{
	INT		Gid;					// Group Id
	INT		Slots[2];				// # of slots left for this group
	INT		Users;					// # of users in this group
	CHAR	szDescription[128 + 1];	// Long description
	CHAR	szVfsFile[MAX_PATH + 1];	// Default VFS file

	LPVOID	lpInternal;
	LPVOID	lpParent;

} GROUPFILE, * LPGROUPFILE;