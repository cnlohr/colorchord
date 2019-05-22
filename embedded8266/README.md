# ESP8266 ColorChord

## WARNING THIS IS UNDER CONSTRUCTION

(based off of ESP8266 I2S WS2812 Driver)

This project is based off of the I2S interface for the mp3 player found here:
https://github.com/espressif/esp8266_mp3_decoder/

If you want more information about the build environment, etc.  You should
check out the regular WS2812 driver, found here: https://github.com/cnlohr/esp8266ws2812i2s

WARNING: This subproject is very jankey!  It's about stable, but I don't think it's quite there yet.

## Hardware Connection

Unfortunately the I2S Out (WS2812 in) pin is the same as RX1 (pin 25), which means if you are programming via the UART, it'll need to be unplugged any time you're testing.  The positive side of this is that it is a pin that is exposed on most ESP8266 breakout boards.

The audio data is taken from TOUT, but must be kept between 0 and 1V.

An option that has been thurroughly tested is for use with the 2019 MAGFest Swadge.  https://github.com/cnlohr/swadge2019

Audio portion:
![Audio portion of schematic](https://raw.githubusercontent.com/cnlohr/swadge2019/master/hardware/swadge2019_schematic_audio.png)

## Notes

./makeconf.inc has a few variables that Make uses for building and flashing the firmware.
Most notably the location of the toolchain for the esp8266/85 on your system.
You should edit them according to your preferences or better add `export ESP_ROOT='/path/to/the/esp-open-sdk'` to your bashrc.

If you have problems with burning the firmware or transfering page data over network (`make netburn` or `make netweb`), you should try uncommenting

    OPTS += -DVERIFY_FLASH_WRITE

in `makeconf.inc`. This way the esp checks if the flash is written correctly.
Especially with some ESP-01 modules there has been a problem with the flash
not being written correctly.

## UDP Commands

These commands can be sent to port 7878, defined in user.cfg. Custom commands from custom_commands.c all start with 'C'. All others from commonservices.c do not. The non-custom commands reference can be found at https://github.com/cnlohr/esp82xx#commands

| Command | Name | Description |
| -------------- | ---- | ----------- |
| CB | Bins | Given an integer, return the bins vals in a string over UDP. 0 == embeddedBins32, 1 == fizzed_bins, 3 == folded_bins |
| CL | LEDs | Return the LED values in a string over UDP |
| CM | Oscilloscope | Return the sounddata values in a string over UDP |
| CN | Notes | Return the note peak frequency, peak amplitudes, and jumps in a string over UDP |
| CSD | Config Default | Set all configurables to default values | 
| CSR | Config Read | Read all configurables from struct SaveLoad |
| CSS | Config Save | Write all configurables to SPI flash |
| CVR | Colorchord Values Read | Return all configurables in string form over UDP |
| CVW | Colorchord Values Write | Given a name and value pair, set a configurable |

Also there's a UDP server on port 7777 which can set the LEDs. Just send it an array of raw bytes for the LEDs in RGB order. So index 0 is LED1_R, index 1 is LED1_G, index 2 is LED1_B, index 3 is LED2_R, etc. It seems to ignore the first three bytes sent (first LED), but reads three bytes past where the data ends, so that may be a bug.
