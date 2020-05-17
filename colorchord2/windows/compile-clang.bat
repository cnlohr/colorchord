set CC="C:\Program Files\LLVM\bin\clang.exe"
set CCFLAGS=-g -D_CRT_SECURE_NO_WARNINGS -Wno-deprecated-declarations -DICACHE_FLASH_ATTR= -Dstrdup=_strdup
set CCIFLAGS=-I../../embeddedcommon -I../cnfa -I../rawdraw -I../ -O1
set CCLFLAGS=-lwinmm -lgdi32 -lws2_32 -lsetupapi -lkernel32 -luser32 -ldbghelp -lole32 -lmmdevapi -lAvrt
set SOURCES=../main.c  ../dft.c ../decompose.c ../filter.c ../color.c ../notefinder.c ../util.c ../outdrivers.c ^
../parameters.c ../chash.c ../OutputVoronoi.c ../OutputProminent.c ../DisplayArray.c ^
../OutputLinear.c ../DisplayPie.c ../DisplayNetwork.c ../hook.c ../RecorderPlugin.c ^
../../embeddedcommon/DFT32.c ../OutputCells.c ../configs.c ../hidapi.c ../DisplayHIDAPI.c

@echo on
%CC% %CCFLAGS% %CCIFLAGS% -o ../colorchord.exe %SOURCES% %CCLFLAGS%
