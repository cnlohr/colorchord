@echo off
set CC="C:\Program Files\LLVM\bin\clang.exe"
rem To enable OpenGL rendering use the -DCNFGOGL option
set CCFLAGS=-g -D_CRT_SECURE_NO_WARNINGS -DCNFGOGL
set CCIFLAGS=-I../../embeddedcommon -I../cnfa -I../rawdraw -I../ -O2
set CCLFLAGS=-lwinmm -lgdi32 -lws2_32 -lsetupapi -lkernel32 -luser32 -ldbghelp -lole32 -lmmdevapi -lAvrt -lopengl32
set SOURCES=..\main.c ..\chash.c ..\color.c ..\configs.c ..\decompose.c ..\dft.c ..\filter.c ^
..\outdrivers.c ..\hidapi.c ..\hook.c   ..\parameters.c ..\util.c ..\notefinder.c ^
..\..\embeddedcommon\DFT32.c symbol_enumerator.c ^
..\DisplayArray.c ..\DisplayDMX.c ..\DisplayFileWrite.c ..\DisplayHIDAPI.c ..\DisplayNetwork.c ^
..\DisplayOUTDriver.c ..\DisplayPie.c ..\DisplayRadialPoles.c ..\DisplaySHM.c ..\DisplayUSB2812.c ^
..\OutputCells.c ..\OutputLinear.c ..\OutputProminent.c ..\OutputVoronoi.c .

@echo on
%CC% %CCFLAGS% %CCIFLAGS% -o ../colorchord.exe %SOURCES% %CCLFLAGS%
