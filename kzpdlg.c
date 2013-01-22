#define INCL_PM
#include <os2.h>

#include "kzpdlg.h"

static OPTDLGPARAM param = { FALSE, FALSE };
static POPTDLGPARAM result = NULL;

static MRESULT optDlg_wmInitDlg( HWND, MPARAM, MPARAM );
static MRESULT optDlg_wmControl( HWND, MPARAM, MPARAM );
static MRESULT optDlg_wmCommand( HWND, MPARAM, MPARAM );

MRESULT EXPENTRY optDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg )
    {
        case WM_INITDLG : return optDlg_wmInitDlg( hwnd, mp1, mp2 );
        case WM_CONTROL : return optDlg_wmControl( hwnd, mp1, mp2 );
        case WM_COMMAND : return optDlg_wmCommand( hwnd, mp1, mp2 );

    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

MRESULT optDlg_wmInitDlg( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    LONG            cxScreen, cyScreen;
    RECTL           rcl;
    HWND            hwnd3bul = WinWindowFromID( hwnd, IDB_3BUL );
    HWND            hwndChat = WinWindowFromID( hwnd, IDB_CHATLINE );

    result = PVOIDFROMMP( mp2 );
    param.patch3bul = result->patch3bul;
    param.patchChat = result->patchChat;

    WinSendMsg( hwnd3bul, BM_SETCHECK, MPFROMLONG( param.patch3bul ), 0 );
    WinSendMsg( hwndChat, BM_SETCHECK, MPFROMLONG( param.patchChat ), 0 );

    cxScreen = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
    cyScreen = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );

    WinQueryWindowRect( hwnd, &rcl );

    WinSetWindowPos( hwnd, HWND_TOP,
                     ( cxScreen - ( rcl.xRight - rcl.xLeft )) / 2,
                     ( cyScreen - ( rcl.yTop - rcl.yBottom )) / 2,
                     0, 0, SWP_MOVE );

    return 0;
}

MRESULT optDlg_wmControl( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    USHORT id = SHORT1FROMMP( mp1 );
    USHORT notifyCode = SHORT2FROMMP( mp1 );

    switch( id )
    {
        case IDB_3BUL :
            if( notifyCode == BN_CLICKED )
                param.patch3bul = LONGFROMMR( WinSendDlgItemMsg( hwnd, id, BM_QUERYCHECK, 0, 0 ));
            break;

        case IDB_CHATLINE :
            if( notifyCode == BN_CLICKED )
                param.patchChat = LONGFROMMR( WinSendDlgItemMsg( hwnd, id, BM_QUERYCHECK, 0, 0 ));
            break;
    }

    return 0;
}

MRESULT optDlg_wmCommand( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    if( SHORT1FROMMP( mp1 ) == DID_OK )
    {
        result->patch3bul = param.patch3bul;
        result->patchChat = param.patchChat;

        WinDismissDlg( hwnd, DID_OK );
    }
    else
        return WinDefDlgProc( hwnd, WM_COMMAND, mp1, mp2 );

    return 0;
}

