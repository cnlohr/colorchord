@echo off
echo Unzip http://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip to C:\tcc
echo Don't worry.  It includes the i386 compiler in the win64 build.
set CFLAGS=-v -DHIDAPI -DWINDOWS -DWIN32 -DTCC -DRUNTIME_SYMNUM -Os -Itccinc -DINCLUDING_EMBEDDED -I.. -I. -I../../embeddedcommon -rdynamic -g
set LDFLAGS=-lkernel32 -lgdi32 -luser32 -lsetupapi -ldbghelp -lws2_32
set SOURCES=..\chash.c ..\color.c ..\configs.c ..\decompose.c ..\dft.c ..\DisplayNetwork.c ..\DisplayArray.c ..\DisplayHIDAPI.c ..\DisplayOUTDriver.c ..\DisplayPie.c ..\DrawFunctions.c ..\filter.c ..\hidapi.c ..\hook.c ..\main.c ..\os_generic.c ..\outdrivers.c ..\OutputCells.c ..\OutputLinear.c ..\OutputProminent.c ..\OutputVoronoi.c ..\parameters.c ..\sound.c ..\sound_win.c ..\sound_null.c ..\util.c ..\WinDriver.c ..\notefinder.c ..\..\embeddedcommon\DFT32.c tcc_stubs.c symbol_enumerator.c
set ARCH_SPECIFIC=-L32 C:\windows\system32\winmm.dll
set CC=C:\tcc\i386-win32-tcc.exe
rem set CC=C:\tcc\x86_64-win32-tcc.exe
@echo on
%CC% %CFLAGS% %ARCH_SPECIFIC% %SOURCES% %LDFLAGS% -o ..\colorchord.exe
pause
