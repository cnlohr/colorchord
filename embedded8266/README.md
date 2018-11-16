#ESP8266 ColorChord

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

## Notes

./makeconf.inc has a few variables that Make uses for building and flashing the firmware.
Most notably the location of the toolchain for the esp8266/85 on your system.
You should edit them according to your preferences or better add `export ESP_ROOT='/path/to/the/esp-open-sdk'` to your bashrc.

If you have problems with burning the firmware or transfering page data over network (`make netburn` or `make netweb`), you should try uncommenting

    OPTS += -DVERIFY_FLASH_WRITE

in `makeconf.inc`. This way the esp checks if the flash is written correctly.
Especially with some ESP-01 modules there has been a problem with the flash
not being written correctly.
