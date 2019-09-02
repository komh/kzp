/*
        DLL for Ztelnet Patch
        Copyright (C) 2001 by KO Myung-Hun

        This library is free software; you can redistribute it and/or
        modify it under the terms of the GNU Library General Public
        License as published by the Free Software Foundation; either
        version 2 of the License, or any later version.

        This library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Library General Public License for more details.

        You should have received a copy of the GNU Library General Public
        License along with this library; if not, write to the Free
        Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Change Log :

        Written by KO Myung-Hun
        Term of programming : 2001.05.25 - 2001.06.01

        Modified by KO Myung-Hun
        Term of programming : 2001.10.09

        Modified contents :
            Can input Korean chars regardless to keyboard layout.

        Source file   : kzphook.c
        Used compiler : emx 0.9d + gcc 2.8.1
*/

#define INCL_DOSMODULEMGR
#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "kzp.h"
#include "kzphook.h"
#include "kzpconv.inc"

#define DLL_NAME    "KZPHOOK"

#define WC_ZTELNET  "ClientWindowClass"

#define SBCS_CHARS  "`1234567890-=\\~!@#$%^&*()_+|[]{};':\",./<>?"

OPTDLGPARAM patchOpt = { TRUE, TRUE };

static HAB hab = 0;
static HWND hwndKzp = NULLHANDLE;

static HMODULE hm = NULLHANDLE;
static UCHAR uchPrevDbl = 0;
static BOOL dblJaumPressed = FALSE;
static BOOL prevHanInput = FALSE;

static BOOL isDblJaum( UCHAR uch );
static BOOL isZtelnet( HWND hwnd );
static BOOL isKzpProcess( HWND hwnd );
static UCHAR *findKbdConv( UCHAR uch );
static USHORT kbdKeyTranslate( PQMSG pQmsg );
static BOOL isCapsLockOn( VOID );

BOOL EXPENTRY inputHook( HAB hab, PQMSG pQmsg, USHORT fsOptions )
{
    if( !isZtelnet( pQmsg->hwnd ))
        return FALSE;

    if( pQmsg->msg == WM_CHAR )
    {
        USHORT  fsFlags = SHORT1FROMMP( pQmsg->mp1 );
        //UCHAR   ucRepeat = CHAR3FROMMP( pQmsg->mp1 );
        //UCHAR   ucScancode = CHAR4FROMMP( pQmsg->mp1 );
        USHORT  usCh = SHORT1FROMMP( pQmsg->mp2 );
        USHORT  usVk = SHORT2FROMMP( pQmsg->mp2 );

        if( fsFlags & KC_KEYUP )
            return FALSE;

        if(( fsFlags & ( KC_SHIFT | KC_CTRL )) && ( usVk == VK_SPACE ))
            WinSendMsg( hwndKzp, KZPM_CHANGEHAN, 0, 0 );
        else if( !( fsFlags & KC_ALT ) && ( usVk == VK_F3 ))
            WinSendMsg( hwndKzp, KZPM_CHANGEIM, 0, 0 );
        else if( LONGFROMMR( WinSendMsg( hwndKzp, KZPM_QUERYHAN, 0, 0 ))
                 && ( !( fsFlags & ( KC_CTRL | KC_ALT ))))
        {
            UCHAR uch;
            UCHAR *kbdConv;
            BOOL  shiftOn;

            usCh = kbdKeyTranslate( pQmsg );
            uch = tolower( LOUCHAR( usCh ));
            if( fsFlags & KC_SHIFT )
                uch = toupper( uch );
            else if( patchOpt.patchChat && prevHanInput &&
                     LONGFROMMR( WinSendMsg( hwndKzp, KZPM_QUERYIM, 0, 0 )) &&
                     ( usVk == VK_SPACE ))
                     WinSendMsg( pQmsg->hwnd, pQmsg->msg, pQmsg->mp1, pQmsg->mp2 );

            shiftOn = FALSE;
            if( isDblJaum( uch ))
            {
                if( dblJaumPressed )
                {
                    if( uchPrevDbl == uch )
                    {
                        if( patchOpt.patch3bul )
                            WinSendMsg( pQmsg->hwnd, WM_CHAR,
                                        MPFROMSH2CH( fsFlags | KC_VIRTUALKEY, 0, 0 ),
                                        MPFROM2SHORT( 0, VK_BACKSPACE ));

                        uchPrevDbl = 0;
                        dblJaumPressed = FALSE;
                        shiftOn = TRUE;
                    }
                    else
                        uchPrevDbl = uch;
                }
                else
                {
                    uchPrevDbl = uch;
                    dblJaumPressed = TRUE;
                }
            }
            else
                dblJaumPressed = FALSE;

            if(( kbdConv = findKbdConv( uch )) != NULL )
            {
                fsFlags &= ~KC_SHIFT;
                if( kbdConv[ 3 ] || shiftOn )
                    fsFlags |= KC_SHIFT;

                usCh = MAKEUSHORT( kbdConv[ 1 ], 0 );
                if( kbdConv[ 2 ])
                {
                    if( patchOpt.patch3bul )
                        WinSendMsg( pQmsg->hwnd, WM_CHAR,
                                    MPFROMSH2CH( fsFlags, 0, 0 ),
                                    MPFROM2SHORT( usCh, 0 ));

                    usCh = MAKEUSHORT( kbdConv[ 2 ], 0 );
                }

                if( usCh )
                {
                    if( patchOpt.patch3bul )
                    {
                        //pQmsg->mp1 = MPFROMSH2CH( fsFlags, ucRepeat, ucScancode );
                        //pQmsg->mp2 = MPFROM2SHORT( usCh, usVk );

                        pQmsg->mp1 = MPFROMSH2CH( fsFlags, 0, 0 );
                        pQmsg->mp2 = MPFROM2SHORT( usCh, 0 );
                    }
                }
            }

            prevHanInput = ( strchr( SBCS_CHARS, SHORT1FROMMP( pQmsg->mp2 )) == NULL ) &&
                           !( fsFlags & KC_VIRTUALKEY );
        }
    }

    return FALSE;
}

VOID EXPENTRY sendMsgHook( HAB hab, PSMHSTRUCT psmh, BOOL fInterTask )
{
    if( !isZtelnet( psmh->hwnd ))
        return;

    if( psmh->msg == WM_SETFOCUS )
    {
        HWND hwnd = HWNDFROMMP( psmh->mp1 );
        BOOL focus = SHORT1FROMMP( psmh->mp2 );

        if( focus )
            WinSetWindowPos( hwndKzp, HWND_TOP, 0, 0, 0, 0, SWP_SHOW | SWP_ZORDER );
        else if( !isKzpProcess( hwnd ))
            WinShowWindow( hwndKzp, FALSE );
    }
    else if( psmh->msg == WM_DESTROY )
        WinShowWindow( hwndKzp, FALSE );
}

BOOL EXPENTRY installHook( HAB habAB, HWND hwnd )
{
    if( DosQueryModuleHandle( DLL_NAME, &hm ) != 0 )
        return FALSE;

    hab = habAB;
    WinSetHook( hab, NULLHANDLE, HK_INPUT, ( PFN )inputHook, hm );
    WinSetHook( hab, NULLHANDLE, HK_SENDMSG, ( PFN )sendMsgHook, hm );

    hwndKzp = hwnd;

    return TRUE;
}

VOID EXPENTRY uninstallHook( VOID )
{
    WinReleaseHook( hab, NULLHANDLE, HK_INPUT, ( PFN )inputHook, hm );
    WinReleaseHook( hab, NULLHANDLE, HK_SENDMSG, ( PFN )sendMsgHook, hm );

    WinBroadcastMsg(HWND_DESKTOP, WM_NULL, 0, 0, BMSG_FRAMEONLY | BMSG_POST);
}

BOOL isDblJaum( UCHAR uch )
{
    return (( uch == 'k' ) || ( uch == 'u' ) || ( uch == ';' ) ||
            ( uch == 'n' ) || ( uch == 'l' ));
}

BOOL isZtelnet( HWND hwnd )
{
    UCHAR szClassName[ 256 ];

    WinQueryClassName( hwnd, sizeof( szClassName ), szClassName );

    return ( strcmp( szClassName, WC_ZTELNET ) == 0 );
}

BOOL isKzpProcess( HWND hwnd )
{
    PID pidKzp, pidHwnd;

    WinQueryWindowProcess( hwndKzp, &pidKzp, NULL );
    WinQueryWindowProcess( hwnd, &pidHwnd, NULL );

    return( pidKzp == pidHwnd );
}

UCHAR *findKbdConv( UCHAR uch )
{
    int i;

    for( i = 0; ( kbdConvTable[ i ][ 0 ] != 0 ) &&
                ( kbdConvTable[ i ][ 0 ] != uch ); i ++ );

    return (( kbdConvTable[ i ][ 0 ] == 0 ) ? NULL : &kbdConvTable[ i ][ 0 ]);
}


USHORT kbdKeyTranslate( PQMSG pQmsg )
{
    USHORT  fsFlags = SHORT1FROMMP( pQmsg->mp1 );
    UCHAR   ucRepeat = CHAR3FROMMP( pQmsg->mp1 );
    UCHAR   ucScancode = CHAR4FROMMP( pQmsg->mp1 );
    USHORT  usCh = SHORT1FROMMP( pQmsg->mp2 );
    USHORT  usVk = SHORT2FROMMP( pQmsg->mp2 );

    if( ucScancode < 54 )
    {
        BOOL  shiftOn = ( fsFlags & KC_SHIFT ) ? TRUE : FALSE;
        UCHAR uch = kbdKeyTransTable[ ucScancode ][ shiftOn ];

        if( uch != 0 )
        {
            uch = ( shiftOn ^ isCapsLockOn()) ? toupper( uch ) : tolower( uch );

            usCh = MAKEUSHORT( uch, 0 );

            pQmsg->mp1 = MPFROMSH2CH( fsFlags, ucRepeat, ucScancode );
            pQmsg->mp2 = MPFROM2SHORT( usCh, usVk );
        }
    }

    return usCh;
}

BOOL isCapsLockOn( VOID )
{
    BYTE keyState[ 256 ];

    WinSetKeyboardStateTable( HWND_DESKTOP, keyState, FALSE );

    return ( keyState[ VK_CAPSLOCK ] & 0x01 );
}


