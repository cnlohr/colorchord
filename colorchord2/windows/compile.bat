@echo off
echo Unzip http://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip to C:\tcc
echo Also, if compiling with OpenGL, download http://download.savannah.nongnu.org/releases/tinycc/winapi-full-for-0.9.27.zip and overwrite the include, etc. folders in C:\tcc.
echo Don't worry.  It includes the i386 compiler in the win64 build.

set CFLAGS= -v -DHIDAPI -DWINDOWS -DWIN32 -DTCC -DRUNTIME_SYMNUM -O2 -Itccinc -DINCLUDING_EMBEDDED -rdynamic -g -DCNFGOGL
set INCLUDES=-I../rawdraw -I../cnfa -I.. -I. -I../../embeddedcommon 
set LDFLAGS=-lkernel32 -lole32 -lgdi32 -luser32 -lsetupapi -ldbghelp -lws2_32 -lAvrt -lopengl32

rem lots of source files
set SOURCES=..\main.c ..\chash.c ..\color.c ..\configs.c ..\decompose.c ..\dft.c ..\filter.c ^
..\outdrivers.c ..\hidapi.c ..\hook.c   ..\parameters.c ..\util.c ..\notefinder.c ^
..\..\embeddedcommon\DFT32.c tcc_stubs.c symbol_enumerator.c ^
..\DisplayNetwork.c ..\DisplayArray.c ..\DisplayHIDAPI.c ..\DisplayOUTDriver.c ..\DisplayPie.c ^
..\OutputCells.c ..\OutputLinear.c ..\OutputProminent.c ..\OutputVoronoi.c 
  
set ARCH_SPECIFIC=-L32 C:\windows\system32\winmm.dll -DWIN32_LEAN_AND_MEAN 
set CC=C:\tcc\tcc.exe
rem set CC=C:\tcc\i386-win32-tcc.exe
rem set CC=C:\tcc\x86_64-win32-tcc.exe
@echo on
%CC% %CFLAGS% %INCLUDES% %ARCH_SPECIFIC% %SOURCES% %LDFLAGS% -o ..\colorchord.exe
@echo off
pause
