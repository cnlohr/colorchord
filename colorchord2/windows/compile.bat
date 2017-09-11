@echo off
echo Unzip https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.26-win64-bin.zip to C:\tcc
set CFLAGS=-v -DHIDAPI -DWINDOWS -DWIN32 -DTCC -DRUNTIME_SYMNUM -Os -Itccinc -DINCLUDING_EMBEDDED -I.. -I. -I../../embeddedcommon -rdynamic -g
set LDFLAGS=-lkernel32 -lgdi32 -luser32 -lsetupapi -ldbghelp -ltcc1 -lwinmm -lws2_32
set SOURCES=..\chash.c ..\color.c ..\configs.c ..\decompose.c ..\dft.c ..\DisplayNetwork.c ..\DisplayArray.c ..\DisplayHIDAPI.c ..\DisplayOUTDriver.c ..\DisplayPie.c ..\DrawFunctions.c ..\filter.c ..\hidapi.c ..\hook.c ..\main.c ..\os_generic.c ..\outdrivers.c ..\OutputCells.c ..\OutputLinear.c ..\OutputProminent.c ..\OutputVoronoi.c ..\parameters.c ..\sound.c ..\sound_win.c ..\sound_null.c ..\util.c ..\WinDriver.c ..\notefinder.c ..\..\embeddedcommon\DFT32.c tcc_stubs.c symbol_enumerator.c
set ARCH_SPECIFIC=-L32
@echo on
C:\tcc\tcc %CFLAGS% %ARCH_SPECIFIC% %SOURCES% %LDFLAGS% -o ..\colorchord.exe 