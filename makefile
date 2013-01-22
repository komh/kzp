EXE_NAME   = kzp.exe
SRC_LIST   = kzp.c kzpdlg.c
RC_LIST    = kzp.rc
A_LIST     =
LIB_LIST   = kzphook.lib

.INCLUDE : "makefile.inc"

kzp.obj : kzp.h

kzphook.lib : kzphook.dll kzphook.def
    $(IMP) -o $@ kzphook.def

kzphook.dll : kzphook.obj kzphook.def
    $(CC) $(LFDLL) $&

kzphook.obj : kzphook.h

kzpdlg.obj : kzpdlg.h
