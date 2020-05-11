
APPNAME=colorchord
RAWDRAWANDROID=rawdrawandroid
PACKAGENAME?=org.cnlohr.$(APPNAME)

CFLAGS:=-I. -I.. -Irawdrawandroid/rawdraw -I../cnfa -I../../embeddedcommon -ffunction-sections -Os -DPRINTF_NO_OVERRIDDE
LDFLAGS:=-s -lOpenSLES 
PACKAGENAME?=org.yourorgexample.$(APPNAME)

CC_C:=  ../main.c  ../dft.c ../decompose.c ../filter.c ../color.c ../notefinder.c ../util.c ../hook.c ../outdrivers.c ../parameters.c ../chash.c ../OutputVoronoi.c ../OutputProminent.c ../DisplayArray.c ../OutputLinear.c ../DisplayPie.c ../DisplayNetwork.c ../../embeddedcommon/DFT32.c ../OutputCells.c ../configs.c

SRC:=rawdrawandroid/android_usb_devices.c DisplayTensigralAndroid.c $(CC_C)

include rawdrawandroid/Makefile


