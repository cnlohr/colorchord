@echo off
set CC="C:\Program Files\LLVM\bin\clang.exe"
rem To enable OpenGL rendering use the -DCNFGOGL option
set CCFLAGS=-g -D_CRT_SECURE_NO_WARNINGS 
set CCIFLAGS=-I../../embeddedcommon -I../cnfa -I../rawdraw -I../ -O2
set CCLFLAGS=-lwinmm -lgdi32 -lws2_32 -lsetupapi -lkernel32 -luser32 -ldbghelp -lole32 -lmmdevapi -lAvrt
set SOURCES=../main.c  ../dft.c ../decompose.c ../filter.c ../color.c ../notefinder.c ../util.c ../outdrivers.c ^
../parameters.c ../chash.c ../OutputVoronoi.c ../OutputProminent.c ../DisplayArray.c ^
../OutputLinear.c ../DisplayPie.c ../DisplayNetwork.c ../hook.c ../RecorderPlugin.c ^
../../embeddedcommon/DFT32.c ../OutputCells.c ../configs.c ../hidapi.c ../DisplayHIDAPI.c

@echo on
%CC% %CCFLAGS% %CCIFLAGS% -o ../colorchord.exe %SOURCES% %CCLFLAGS%
