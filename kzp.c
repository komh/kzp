// 2001.05.25
#define INCL_DOSPROCESS
#define INCL_WIN
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include "kzp.h"
#include "kzpdlg.h"
#include "kzphook.h"

#define KZP_NAME    "K Ztelnet Patch"

#define BUTTON_COUNT    2               // han/eng, char/line
#define BORDER_SIZE     1

#define EXTRA_WIDTH     1
#define EXTRA_HEIGHT    1
#define WIN_WIDTH       (( BORDER_SIZE * 2 ) + ( charWidth * 3 ) + ( EXTRA_WIDTH * 2 ) +\
                         ( charWidth * 4 ) + ( EXTRA_WIDTH * 2 ))
#define WIN_HEIGHT      (( BORDER_SIZE * 2 ) + charHeight * 2 + ( EXTRA_HEIGHT * 2 ))

typedef struct tagKZP {
    BOOL    han;
    BOOL    line;
} KZP, *PKZP;

static const char *hanStatusStr[] = { "Eng", "Han", };
static const char *imStatusStr[] = { "Char", "Line", };

static HWND hwndPopup = NULLHANDLE;

static PFNWP oldButtonWndProc = NULL;

static VOID run( HAB );
static BOOL alreadyLoaded( HAB, CHAR * );
static VOID processArg( INT, CHAR ** );
static VOID queryCharSize( HWND, LONG *, LONG * );

static MRESULT EXPENTRY newButtonWndProc( HWND, ULONG, MPARAM, MPARAM );

static MRESULT EXPENTRY windowProc( HWND, ULONG, MPARAM, MPARAM );

static MRESULT wmCreate( HWND, MPARAM, MPARAM );
static MRESULT wmDestroy( HWND, MPARAM, MPARAM );
static MRESULT wmSize( HWND, MPARAM, MPARAM );
static MRESULT wmPaint( HWND, MPARAM, MPARAM );
static MRESULT wmBeginDrag( HWND, MPARAM, MPARAM );
static MRESULT wmButton2Up( HWND, MPARAM, MPARAM );
static MRESULT wmCommand( HWND, MPARAM, MPARAM );

static MRESULT kzp_umChangeHan( HWND, MPARAM, MPARAM );
static MRESULT kzp_umSetHan( HWND, MPARAM, MPARAM );
static MRESULT kzp_umQueryHan( HWND, MPARAM, MPARAM );
static MRESULT kzp_umChangeIm( HWND, MPARAM, MPARAM );
static MRESULT kzp_umSetIm( HWND, MPARAM, MPARAM );
static MRESULT kzp_umQueryIm( HWND, MPARAM, MPARAM );

INT main( int argc, char **argv )
{
    HAB     hab;

    processArg( argc, argv );

    hab = WinInitialize( 0 );


    if( !alreadyLoaded( hab, KZP_NAME ))
        run( hab );

    WinTerminate( hab );

    return 0;
}

VOID run( HAB hab )
{
    HMQ     hmq;
    HWND    hwnd;
    QMSG    qm;

    hmq = WinCreateMsgQueue( hab, 0);

    WinRegisterClass(
        hab,
        WC_KZP,
        windowProc,
        CS_SIZEREDRAW,
        sizeof( PVOID )
    );

    hwnd = WinCreateWindow( HWND_DESKTOP, WC_KZP, KZP_NAME, WS_VISIBLE,
                            0, 0, 0, 0, NULLHANDLE, HWND_TOP, ID_KZP,
                            NULL, NULL );

    if( hwnd != NULLHANDLE )
    {
        LONG charWidth, charHeight;

        queryCharSize( hwnd, &charWidth, &charHeight );

        WinSetWindowPos( hwnd, HWND_TOP, 0, 0, WIN_WIDTH, WIN_HEIGHT, SWP_MOVE | SWP_SIZE | SWP_HIDE );

        while( WinGetMsg( hab, &qm, NULLHANDLE, 0, 0 ))
            WinDispatchMsg( hab, &qm );

        WinDestroyWindow( hwnd );
    }

    WinDestroyMsgQueue( hmq );

}

BOOL alreadyLoaded( HAB hab, CHAR *szAppName )
{
    PPIB        pib;
    PSWBLOCK    swb;
    ULONG       count;
    ULONG       bufSize;
    INT         i;
    BOOL        result;

    count = WinQuerySwitchList( hab, NULL, 0 );
    bufSize = ( count * sizeof( SWENTRY )) + sizeof( ULONG );

    swb = ( PSWBLOCK )malloc( bufSize );
    WinQuerySwitchList( hab, swb, bufSize );

    DosGetInfoBlocks( NULL, &pib );

    result = FALSE;
    for( i = 0; i < swb->cswentry; i ++ )
    {
        if(( strcmp( swb->aswentry[ i ].swctl.szSwtitle, szAppName ) == 0 ) &&
           ( swb->aswentry[ i ].swctl.idProcess != pib->pib_ulpid ))
        {
            result = TRUE;
            break;
        }
    }

    free( swb );

    return result;
}

VOID processArg( INT argc, CHAR **argv )
{
    INT i;

    for( i = 1; i < argc; i ++ )
    {
        if( strcmp( argv[ i ], "--no-3bul" ) == 0 )
            patchOpt.patch3bul = FALSE;
        else if( strcmp( argv[ i ], "--no-chatline" ) == 0 )
            patchOpt.patchChat = FALSE;
    }
}

VOID queryCharSize( HWND hwnd, LONG *charWidth, LONG *charHeight )
{
    HPS         hps;
    FONTMETRICS fm;

    hps = WinGetPS( hwnd );

    GpiQueryFontMetrics( hps, sizeof( FONTMETRICS ), &fm );

    WinReleasePS( hps );

    *charWidth = fm.lAveCharWidth + fm.lMaxCharInc / 2;
    *charHeight = fm.lMaxAscender + fm.lMaxDescender;
}

MRESULT EXPENTRY newButtonWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    if( msg == WM_BEGINDRAG )
    {
        HWND hwndOwner = WinQueryWindow( hwnd, QW_OWNER );

        if( hwndOwner != NULLHANDLE )
        {
            WinPostMsg( hwndOwner, msg, mp1, mp2 );

            return MRFROMLONG( TRUE );
        }
    }

    return oldButtonWndProc( hwnd, msg, mp1,mp2 );
}

MRESULT EXPENTRY windowProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg )
    {
        case WM_CREATE          : return wmCreate( hwnd, mp1, mp2 );
        case WM_DESTROY         : return wmDestroy( hwnd, mp1, mp2 );
        case WM_SIZE            : return wmSize( hwnd, mp1, mp2 );
        case WM_PAINT           : return wmPaint( hwnd, mp1, mp2 );
        case WM_BEGINDRAG       : return wmBeginDrag( hwnd, mp1, mp2 );
        case WM_BUTTON2UP       : return wmButton2Up( hwnd, mp1, mp2 );
        case WM_COMMAND         : return wmCommand( hwnd, mp1, mp2 );

        case KZPM_CHANGEHAN     : return kzp_umChangeHan( hwnd, mp1, mp2 );
        case KZPM_SETHAN        : return kzp_umSetHan( hwnd, mp1, mp2 );
        case KZPM_QUERYHAN      : return kzp_umQueryHan( hwnd, mp1, mp2 );
        case KZPM_CHANGEIM      : return kzp_umChangeIm( hwnd, mp1, mp2 );
        case KZPM_SETIM         : return kzp_umSetIm( hwnd, mp1, mp2 );
        case KZPM_QUERYIM       : return kzp_umQueryIm( hwnd, mp1, mp2 );
    }

    return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

MRESULT wmCreate( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp;
    PCREATESTRUCT pcs = ( PCREATESTRUCT )mp2;
    SWCNTRL swc;
    LONG x = BORDER_SIZE * 2;
    LONG y = BORDER_SIZE * 2;
    LONG cx = ( pcs->cx - BORDER_SIZE * 3 * 2 ) / BUTTON_COUNT;
    LONG cy = ( pcs->cy - BORDER_SIZE * 2 * 2 );
    HWND hwndBtn;

    if( !installHook( WinQueryAnchorBlock( hwnd ), hwnd ))
        return MRFROMLONG( TRUE );

    WinSetWindowPtr( hwnd, 0, NULL );

    kzp = malloc( sizeof( KZP ));
    if( kzp == NULL )
        return MRFROMLONG( TRUE );

    kzp->han = FALSE;
    kzp->line = FALSE;
    WinSetWindowPtr( hwnd, 0, kzp );

    swc.hwnd = hwnd;
    swc.hwndIcon = NULLHANDLE;
    swc.hprog = NULLHANDLE;
    swc.idProcess = 0;
    swc.idSession = 0;
    swc.uchVisibility = SWL_VISIBLE;
    swc.fbJump = SWL_JUMPABLE;
    swc.bProgType = PROG_DEFAULT;
    strcpy( swc.szSwtitle, pcs->pszText );

    WinAddSwitchEntry( &swc );

    hwndBtn = WinCreateWindow( hwnd, WC_BUTTON, hanStatusStr[ kzp->han ], WS_VISIBLE,
                     x, y, cx, cy, hwnd, HWND_TOP, IDB_HANENG,
                     NULL, NULL );

    oldButtonWndProc = WinSubclassWindow( hwndBtn, newButtonWndProc );

    x += cx + BORDER_SIZE * 2;
    hwndBtn = WinCreateWindow( hwnd, WC_BUTTON, imStatusStr[ kzp->line ], WS_VISIBLE,
                     x, y, cx, cy, hwnd, HWND_TOP, IDB_IM,
                     NULL, NULL );

    oldButtonWndProc = WinSubclassWindow( hwndBtn, newButtonWndProc );


/*
    x += cx;
    WinCreateWindow( hwnd, WC_BUTTON, "KZP", WS_VISIBLE,
                     x, y, cx, cy, hwnd, HWND_TOP, IDB_KZP,
                     NULL, NULL );
*/

    hwndPopup = WinLoadMenu( hwnd, NULLHANDLE, ID_POPUP );

    return 0;
}

MRESULT wmDestroy( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp = WinQueryWindowPtr( hwnd, 0 );

    if( kzp != NULL )
    {
        WinRemoveSwitchEntry( WinQuerySwitchHandle( hwnd, 0 ));

        free( kzp );
    }

    uninstallHook();

    return MRFROMSHORT( FALSE );
}

MRESULT wmSize( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    LONG x = BORDER_SIZE * 2;
    LONG y = BORDER_SIZE * 2;
    LONG cx = ( SHORT1FROMMP( mp2 ) - BORDER_SIZE * 3 * 2 ) / BUTTON_COUNT;
    LONG cy = ( SHORT2FROMMP( mp2 ) - BORDER_SIZE * 2 * 2 );

    WinSetWindowPos( WinWindowFromID( hwnd, IDB_HANENG ), HWND_TOP,
                     x, y, cx, cy, SWP_MOVE | SWP_SIZE );

    x += cx + BORDER_SIZE * 2;
    WinSetWindowPos( WinWindowFromID( hwnd, IDB_IM ), HWND_TOP,
                     x, y, cx, cy, SWP_MOVE | SWP_SIZE );

/*
    x += cx;
    WinSetWindowPos( WinWindowFromID( hwnd, IDB_KZP ), HWND_TOP,
                     x, y, cx, cy, SWP_MOVE | SWP_SIZE );
*/

    return 0;
}

MRESULT wmPaint( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    HPS     hps;
    RECTL   rcl;
    POINTL  ptl;
    POINTL  ptlMid;

    hps = WinBeginPaint( hwnd, NULLHANDLE, &rcl);
    WinQueryWindowRect( hwnd, &rcl );

    ptlMid.x = BORDER_SIZE * 2 + (( rcl.xRight - rcl.xLeft ) - BORDER_SIZE * 3 * 2) / BUTTON_COUNT;
    ptlMid.y = BORDER_SIZE * 2;

//    WinFillRect( hps, &rcl, SYSCLR_BUTTONDARK );

    GpiSetColor( hps, SYSCLR_BUTTONDARK );
    ptl.x = rcl.xRight - 1;
    ptl.y = rcl.yTop - 1;
    GpiMove( hps, &ptl );

    ptl.x = rcl.xLeft;
    GpiLine( hps, &ptl );

    ptl.y = rcl.yBottom;
    GpiLine( hps, &ptl );

    GpiSetColor( hps, SYSCLR_BUTTONMIDDLE );

    ptl.x = rcl.xLeft + 1;
    GpiMove( hps, &ptl );

    ptl.x = rcl.xRight - 1;
    GpiLine( hps, &ptl );

    ptl.y = rcl.yTop - 1;
    GpiLine( hps, &ptl );

    rcl.xLeft += BORDER_SIZE;
    rcl.xRight -= BORDER_SIZE;
    rcl.yBottom += BORDER_SIZE;
    rcl.yTop -= BORDER_SIZE;

    GpiSetColor( hps, SYSCLR_BUTTONMIDDLE );
    ptl.x = rcl.xRight - 1;
    ptl.y = rcl.yTop - 1;
    GpiMove( hps, &ptl );

    ptl.x = rcl.xLeft;
    GpiLine( hps, &ptl );

    ptl.y = rcl.yBottom;
    GpiLine( hps, &ptl );

    GpiSetColor( hps, SYSCLR_BUTTONDARK );

    ptl.x = rcl.xLeft + 1;
    GpiMove( hps, &ptl );

    ptl.x = rcl.xRight - 1;
    GpiLine( hps, &ptl );

    ptl.y = rcl.yTop - 1;
    GpiLine( hps, &ptl );

    GpiSetColor( hps, SYSCLR_BUTTONDARK );

    GpiMove( hps, &ptlMid );
    ptl.x = ptlMid.x;
    ptl.y --;
    GpiLine( hps, &ptl );

    GpiSetColor( hps, SYSCLR_BUTTONMIDDLE );

    ptlMid.x ++;
    GpiMove( hps, &ptlMid );
    ptl.x = ptlMid.x;
    ptl.y++;
    GpiLine( hps, &ptl );

    WinEndPaint( hps );

    return MRFROMSHORT( 0 );
}

MRESULT wmBeginDrag( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    TRACKINFO   trackInfo;
    SWP         swp;

    trackInfo.cxBorder = 1;
    trackInfo.cyBorder = 1;
    trackInfo.cxGrid = 1;
    trackInfo.cyGrid = 1;
    trackInfo.cxKeyboard = 8;
    trackInfo.cyKeyboard = 8;

    WinQueryWindowPos( hwnd, &swp );
    trackInfo.rclTrack.xLeft = swp.x;
    trackInfo.rclTrack.xRight = swp.x + swp.cx;
    trackInfo.rclTrack.yBottom = swp.y;
    trackInfo.rclTrack.yTop = swp.y + swp.cy;

    WinQueryWindowPos( HWND_DESKTOP, &swp );
    trackInfo.rclBoundary.xLeft = swp.x;
    trackInfo.rclBoundary.xRight = swp.x + swp.cx;
    trackInfo.rclBoundary.yBottom = swp.y;
    trackInfo.rclBoundary.yTop = swp.y + swp.cy;

    trackInfo.ptlMinTrackSize.x = 0;
    trackInfo.ptlMinTrackSize.y = 0;

    trackInfo.ptlMaxTrackSize.x = swp.cx;
    trackInfo.ptlMaxTrackSize.y = swp.cy;

    trackInfo.fs = TF_MOVE | TF_STANDARD | TF_ALLINBOUNDARY;

    if ( WinTrackRect( HWND_DESKTOP, NULLHANDLE, &trackInfo ))
        WinSetWindowPos( hwnd, HWND_TOP,
                         trackInfo.rclTrack.xLeft, trackInfo.rclTrack.yBottom, 0, 0,
                         SWP_MOVE | SWP_ZORDER );

    return MRFROMLONG( TRUE );
}

MRESULT wmButton2Up( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    ULONG fs = PU_NONE | PU_KEYBOARD | PU_MOUSEBUTTON1;

    if( WinIsWindowEnabled( hwnd ))
    {
        WinPopupMenu( hwnd, hwnd, hwndPopup,
                      SHORT1FROMMP( mp1 ), SHORT2FROMMP( mp1 ),
                      0, fs );
    }

    return MRFROMLONG( TRUE );
}

MRESULT wmCommand( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    switch( SHORT1FROMMP( mp2 ))
    {
        case CMDSRC_PUSHBUTTON :
            switch( SHORT1FROMMP( mp1 ))
            {
                case IDB_HANENG :
                    WinSendMsg( hwnd, KZPM_CHANGEHAN, 0, 0 );
                    break;

                case IDB_IM :
                    WinSendMsg( hwnd, KZPM_CHANGEIM, 0, 0 );
                    break;

/*
                case IDB_KZP :
                    break;
*/
            }
            break;

        case CMDSRC_MENU :
            switch( SHORT1FROMMP( mp1 ))
            {
                case IDM_HIDE :
                    if( WinQueryFocus( HWND_DESKTOP ) == hwnd )
                        WinSetFocus( HWND_DESKTOP, WinQueryWindow( hwnd, QW_OWNER ));

                    WinShowWindow( hwnd, FALSE );
                    break;

                case IDM_OPTIONS :
                    WinDlgBox( HWND_DESKTOP, hwnd, optDlgProc, NULLHANDLE, IDD_OPTDLG, &patchOpt );
                    break;

                case IDM_EXIT :
                    WinPostMsg( hwnd, WM_QUIT, 0, 0 );
                    break;
            }
            break;
    }

    return 0;
}

MRESULT kzp_umChangeHan( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp = WinQueryWindowPtr( hwnd, 0 );
    HWND hwndHanEngBtn = WinWindowFromID( hwnd, IDB_HANENG );

    kzp->han = !kzp->han;
    WinSetWindowText( hwndHanEngBtn, hanStatusStr[ kzp->han ]);

    return 0;
}

MRESULT kzp_umSetHan( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp = WinQueryWindowPtr( hwnd, 0 );
    HWND hwndHanEngBtn = WinWindowFromID( hwnd, IDB_HANENG );

    kzp->han = LONGFROMMP( mp1 );
    WinSetWindowText( hwndHanEngBtn, hanStatusStr[ kzp->han ]);

    return 0;
}

MRESULT kzp_umQueryHan( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp = WinQueryWindowPtr( hwnd, 0 );

    return MRFROMLONG( kzp->han );
}

MRESULT kzp_umChangeIm( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp = WinQueryWindowPtr( hwnd, 0 );
    HWND hwndImBtn = WinWindowFromID( hwnd, IDB_IM );

    kzp->line = !kzp->line;
    WinSetWindowText( hwndImBtn, imStatusStr[ kzp->line ]);

    return 0;
}

MRESULT kzp_umSetIm( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp = WinQueryWindowPtr( hwnd, 0 );
    HWND hwndImBtn = WinWindowFromID( hwnd, IDB_IM );

    kzp->line = LONGFROMMP( mp1 );
    WinSetWindowText( hwndImBtn, imStatusStr[ kzp->line ]);

    return 0;
}

MRESULT kzp_umQueryIm( HWND hwnd, MPARAM mp1, MPARAM mp2 )
{
    PKZP kzp = WinQueryWindowPtr( hwnd, 0 );

    return MRFROMLONG( kzp->line );
}


