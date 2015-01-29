colorchord
==========

What is ColorChord?
-------------------

Chromatic Sound to Light Conversion System.  It's really that simple.  Unlike so many of the sound responsive systems out there, ColorChord looks at the chromatic properties of the sound.  It looks for notes, not ranges.  If it hears an "E" it doesn't care what octave it's in, it's an E.  This provides a good deal more interesting patterns between instruments and music than would be available otherwise.


Background
----------

Developed over many years, ColorChord 2 is now getting close to alpha stages.  ColorChord 2 uses the same principles as ColorChord 1.  A brief writeup on that can be seen here: http://cnlohr.blogspot.com/2010/11/colorchord-sound-lighting.html

The major differences in ColorChord 2 is the major rewrite to move everything back to the CPU and a multitude of algorithmic optimizations to make it possible to run on something other than the brand newest of systems.

TODO: add a link to the new video.

Here's a video of it running: https://www.youtube.com/watch?v=UI4eqOP2AU0

Current State of Affairs
------------------------

Currently, ColorChord 2 is designed to run on Linux.  It's not particularly tied to an architecture, but does pretty much need a dedicated FPU to achieve any decent performance.  Right now there aren't very many output options available for it.  The most interesting one used for debugging is a vornoi-diagram-like thing called "DisplayShapeDriver."

Building and Using
------------------

On Linux you'll need the following packages, for Debian/Ubuntu/Mint you'll need the following:
```
apt-get install libpulse-dev libasound2-dev libx11-dev libxext-dev libxinerama-dev libusb-1.0-0-dev
```

To make colorchord, type:

```
make
```

Then, to run it, use the following syntax:

```
./colorchord [config file, by default 'default.conf'] [any additional parameters]
```

If you edit default.conf while the program is running and resave it, it will use the settings in the newly saved file.


