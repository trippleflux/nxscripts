#ifndef __TOUCH_FLAG_H__
#define __TOUCH_FLAG_H__

#define TOUCH_FLAG_ATIME     0x00000001
#define TOUCH_FLAG_MTIME     0x00000002
#define TOUCH_FLAG_CTIME     0x00000004
#define TOUCH_FLAG_ISDIR     0x00000008
#define TOUCH_FLAG_RECURSE   0x00000010

static BOOL TouchTime(LPCTSTR pszFilePath, LPFILETIME ftTouchTime, WORD wOptions);
static BOOL RecursiveTouch(LPCTSTR pszCurentPath, LPFILETIME ftTouchTime, WORD wOptions);
INT TclTouchCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);

#endif // __TOUCH_FLAG_H__
