
APPNAME=colorchordtest
RAWDRAWANDROID=rawdrawandroid
PACKAGENAME?=com.cnlohr.$(APPNAME)

CFLAGS:=-I. -I.. -Irawdrawandroid/rawdraw -I../cnfa -I../../embeddedcommon -ffunction-sections -Os -DPRINTF_NO_OVERRIDDE
LDFLAGS:=-s -lOpenSLES 

CC_C:=  ../main.c  ../dft.c ../decompose.c ../filter.c ../color.c ../notefinder.c ../util.c ../hook.c ../outdrivers.c ../parameters.c ../chash.c ../OutputVoronoi.c ../OutputProminent.c ../DisplayArray.c ../OutputLinear.c ../DisplayPie.c ../DisplayNetwork.c ../../embeddedcommon/DFT32.c ../OutputCells.c ../configs.c

SRC:=rawdrawandroid/android_usb_devices.c DisplayTensigralAndroid.c $(CC_C)

#Uncomment to make all targets.
TARGETS:=makecapk/lib/arm64-v8a/lib$(APPNAME).so makecapk/lib/armeabi-v7a/lib$(APPNAME).so makecapk/lib/x86/lib$(APPNAME).so makecapk/lib/x86_64/lib$(APPNAME).so


include rawdrawandroid/Makefile


