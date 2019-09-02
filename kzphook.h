// 2001.05.25
#ifndef __KZPHOOK_H__
#define __KZPHOOK_H__

#define INCL_PM
#include <os2.h>

#include "kzpdlg.h"

#ifdef __cplusplus
extern "C" {
#endif

extern OPTDLGPARAM patchOpt;

BOOL EXPENTRY installHook( HAB, HWND );
VOID EXPENTRY uninstallHook( VOID );

#ifdef __cplusplus
}
#endif

#endif

