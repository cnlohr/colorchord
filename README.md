ColorChord
==========

What is ColorChord?
-------------------

Chromatic Sound to Light Conversion System.  It's really that simple.  Unlike so many of the sound responsive systems out there, ColorChord looks at the chromatic properties of the sound.  It looks for notes, not ranges.  If it hears an "E" it doesn't care what octave it's in, it's an E.  This provides a good deal more interesting patterns between instruments and music than would be available otherwise.

Background Video here:

[![What Is ColorChord?](https://img.youtube.com/vi/qE3zEM8qM00/0.jpg)](https://www.youtube.com/watch?v=qE3zEM8qM00)

ColorChord on an ESP8266:

[![Websockets for Awesome on an ESP8266](https://img.youtube.com/vi/8ISbmQTbjDI/0.jpg)](https://www.youtube.com/watch?v=8ISbmQTbjDI)

More videos below!

Background
----------

Developed over many years, ColorChord 2 is now at the alpha stages.  ColorChord 2 uses the same principles as ColorChord 1.  A brief writeup on that can be seen here: http://cnlohr.blogspot.com/2010/11/colorchord-sound-lighting.html

The major differences in ColorChord 2 is the major rewrite to move everything back to the CPU and a multitude of algorithmic optimizations to make it possible to run on something other than the brand newest of systems.

Feuge in D Minor (ColorChord 2 running a strip of WS2812 LEDs):

[![ColorChord](https://img.youtube.com/vi/s8qWqYTpCN4/0.jpg)](https://www.youtube.com/watch?v=s8qWqYTpCN4)

ColorChord 2 running a voronoi diagram with Mayhem's Dr. Rocker

[![ColorChord 2 Preview: Dr Rocker by Mayhem](https://img.youtube.com/vi/UI4eqOP2AU0/0.jpg)](https://www.youtube.com/watch?v=UI4eqOP2AU0)


Current State of Affairs
------------------------

Currently, ColorChord 2 is designed to run on Linux or Windows.  It's not particularly tied to an architecture, but does pretty much need a dedicated FPU to achieve any decent performance.  Right now there aren't very many output options available for it.  The most interesting one used for debugging is a vornoi-diagram-like thing called "DisplayShapeDriver."

ColorChord: Embedded
--------------------

There is work on an embedded version of ColorChord, which avoids floating point operations anywhere in the output pipeline.  Though I have made efforts to port it to AVRs, it doesn't seem feasable to operate on AVRs without some shifty tricks which I'd like to avoid, so I have retargeted my efforts to 32-bit systems, such as the STM32F303, STM32F407, and (somehow) the ESP8266.  ColorChord Embedded uses a different codebase, located in the [embeddedcommon](embeddedcommon/) and distributed among the various embedded* folders.


Building with Linux
-----------------
Use `apt-get` to install the following packages for Debian/Ubuntu/Mint:
```
apt-get install libpulse-dev libasound2-dev libx11-dev libxext-dev libxinerama-dev libusb-1.0-0-dev
```

To make colorchord, type:

```
make
```
Building with Windows
-------------------

With either 64bit or 32bit [MSYS2](https://msys2.github.io/) installed, run the _MSYS2 MSYS_ launcher and use `pacman` to set up a MinGW32 toolchain, if you don't have one already:
```
pacman -S mingw-w64-i686-toolchain
```
If you see "`/mingw32 exists in filesystem`", you must temporarily rename or relocate the _mingw32.exe_ file at MSYS2's root folder level and try again. Restore _mingw32.exe_ before moving on.

Next, run the _MSYS2 MinGW 32bit_ launcher to access the toolchain. The previously launched _MSYS_ terminal can be closed.

To make colorchord, navigate to your working copy and type:
```
mingw32-make colorchord.exe
```

Using
-----

To run colorchord, use the following syntax:

```
./colorchord [config file, by default 'default.conf'] [any additional parameters]
```

If you edit default.conf while the program is running and resave it, it will use the settings in the newly saved file.


Additional Videos
-----------------

[![Come Thou Fount of Every Blessing](https://img.youtube.com/vi/aaB4Afyh5z0/0.jpg)](https://www.youtube.com/watch?v=aaB4Afyh5z0)

[![ColorChord 2 - David Chapman Plays Sax](https://img.youtube.com/vi/DYoK4cZoZ_0/0.jpg)](https://www.youtube.com/watch?v=DYoK4cZoZ_0)


