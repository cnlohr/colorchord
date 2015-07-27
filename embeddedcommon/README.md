ColorChord: Embedded
====================

WARNING: THIS README IS VERY ROUGH AND INCOMPLETE.


What is ColorChord: Embedded
----------------------------

The original ColorChord used a very large amount of CPU in order to operate, on the order of Octaves*BinsPerOctave*FrameHistory every single frame!  This worked out to so much work it wouldn't even work on CPUs of that era (~Pentium 4), and had to operate on the GPU.  ColorChord2 makes use of some logical speedup mechanisms, but still is geared more toward making code more readable, and runtime configurable.  It uses a lot of floating point and has a fair bit of overhead, though it can easily run on modern CPUs.  ColorChord embedded uses the same algorithms, but rips out a lot of the configurability, requiring much of the configuring done at compile time.  Though there is significantly less flexability, it is tight enough to run at ~12ksps on a 50 MHz STM32F303 (at the time of this writing).

What's missing?
---------------

ColorChord: Embedded strips out a lot of features.  One of them is the variety of note finders that are available in ColorChord2.  It only has the 32-bit DFT.  Though the outputs in ColorChord2 are highly configurable, the embedded version only comes with one output.  Many of the parameters are preprocessor macros.

What can I change?
------------------

There are several parameters that can be controlled using preprocessor macros.  They are listed below.  Default values are set for most of them, though others you _must_ provide. 

Section 1, What you must provide:
 * CCEMBEDDED - You must have this #define'd to use any ColorChord: Embedded.
 * DFREQ - The sample rate in samples per second.  Values between 8000 and 16000 are good.

Section 2:, What you can optionally provide:
 * NUM_LIN_LEDS = 32 - Number of LEDs that will be used for output.
 * OCTAVES = 5 - Number of octaves to check against.  This defines the full range of where the algorithm will listen.  As a note, the first and alst octaves are tapered.
 * FIXBPERO = 24 - Number of bins per octave.  Higher values will use more cpu, but can help resolve notes.  24 is chosen as a good medium.  If you use 12, then if there are two adjacent notes (or even ones that are two apart), they will be missed and muddied by the algorithm.
 * DFTIIR = 6 - The response IIR coefficient for the note finder.  Lower values will make ColorChord more responsive, but also more schizophrenic.  There are practical upper limits on this.  Even at 6, if you use the full-range for input signals, it is possible you may overflow the IIR.  You'll have to experiment with pure tones to make sure you don't overflow.


What is the IIR thing here?
---------------------------

We use bitwise IIRs.  [SECTION TODO]


Platforms
---------
Linux - Not really so practical as much as it's just a test for ColorChord.  You can use this to debug changes.  It's convenient to use GDB, have a high speed printf, ability to watch performance on a lot of things, etc.

STM32F303 - Using a STM32F303 with an analog input on the processor.  This implementation expects a 25MHz HSE crystal, with a 2x PLL, so the effective clock rate is 50 MHz.  It doesn't have much free processor, but you could do some simple tasks provided you don't have too many LEDs.  This uses SPI to output to WS2812 LEDs.

STM32F407 - Using a STM32F407 with a PDM microphone.  This implementation expects an 8MHz HSE crystal.  The effective clock rate is 100 MHz.  It has tons of free processor.  This also uses SPI to output to WS2812 LEDs.

ESP8266 - Using an ESP8266 and the on-processor analog in.  It operates at the normal 80 MHz (not double-clocked!) Then it uses I2S to send the WS2812 data.


TODO
----

ColorChord: Embedded could still be optimized, especially for memory usage.  It could also be made more run-time configurable without surrendering much in the way of performance.  At the time of writing this, there is still some floating point being done in the setup.  That should be removed to prevent dependence on any floating point whatsoever.
