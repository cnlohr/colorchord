# ColorChord running on an STM32F303


## Step 1: Get the necessiary build enivronment

```
sudo apt-get install build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi openocd
```

## Step 2: Build

```
make
```

Note: Step 2 being called "build" is a misnomer. If you have your board plugged in, it will also flash.
