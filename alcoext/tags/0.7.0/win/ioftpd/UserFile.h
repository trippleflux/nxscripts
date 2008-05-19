typedef struct _USERFILE
{
	INT				Uid;					// User id
	INT				Gid;					// User group id

	CHAR			Tagline[128 + 1];		// Info line
	CHAR			MountFile[_MAX_PATH + 1];	// Root directory
	CHAR			Home[_MAX_PATH + 1];	// Home directory
	CHAR			Flags[32 + 1];			// Flags
	INT				Limits[5];				//	Up max speed, dn max speed, ftp logins, telnet, http

	UCHAR			Password[20];			// Password

	INT				Ratio[MAX_SECTIONS];	// Ratio
	INT64			Credits[MAX_SECTIONS];	// Credits

	INT64			DayUp[MAX_SECTIONS * 3];	// Daily uploads
	INT64			DayDn[MAX_SECTIONS * 3];	// Daily downloads
	INT64			WkUp[MAX_SECTIONS * 3];		// Weekly uploads
	INT64			WkDn[MAX_SECTIONS * 3];		// Weekly downloads
	INT64			MonthUp[MAX_SECTIONS * 3];	// Monthly uploads
	INT64			MonthDn[MAX_SECTIONS * 3];	// Monthly downloads
	INT64			AllUp[MAX_SECTIONS * 3];	// Alltime uploads
	INT64			AllDn[MAX_SECTIONS * 3];	// Alltime downloads

	INT				AdminGroups[MAX_GROUPS];	// Admin for these groups
	INT				Groups[MAX_GROUPS];					// List of groups
	CHAR			Ip[MAX_IPS][_IP_LINE_LENGTH + 1];	// List of ips

	LPVOID			lpInternal;
	LPVOID			lpParent;

} USERFILE, * PUSERFILE, * LPUSERFILE;

