#ifndef __KZPDLG_H__
#define __KZPDLG_H__

#define IDD_OPTDLG       200
#define IDG_PATCHGROUP   201
#define IDB_3BUL         202
#define IDB_CHATLINE     203

typedef struct tagOPTDLGPARAM {
    BOOL    patch3bul;
    BOOL    patchChat;
} OPTDLGPARAM, *POPTDLGPARAM;

MRESULT EXPENTRY optDlgProc( HWND, ULONG, MPARAM, MPARAM );

#endif
