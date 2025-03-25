# Firmware for the SDPWM paper

## Introduction

This is the firmware accompanying the paper “Improving PWM output from
common MCUs with a Sigma-Delta stage”, now available as a
[preprint](https://danielwagenaar.net).

## Circuit diagram

For testing purposes, the circuit is simple enough that it can be built on a breadboard.

![Breadboard](https://raw.githubusercontent.com/wagenadl/sdpwm-firmware/main/breadboard.png?token=GHSAT0AAAAAADAV27BAQILMOWL6WJPSYTTAZ7DE3YA)

(Created with [Fritzing](https://fritzing.org/).)

## Firmware installation

### Binary installation

The image of the firmware may be downloaded from
[github](https://github.com/wagenadl/sdpwm-firmware/releases/latest) and copied
onto your Raspberry Pi Pico in the usual way: hold down the “Bootsel”
button while powering up the device, then drag the “uf2” onto the
window that opens on your host computer.

### Compiling from source

You will need the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) and [CMake](https://cmake.org).

The source code may be downloaded by

    git clone https://github.com/wagenadl/sdpwm-firmware
    
After that,

    cd sdpwm
    make
    
or, if your operating system does not have “make”, 

    cmake -S . -B build
    cmake --build build
    
This will leave a “uf2” file in the “build” folder, which you can drag
onto a Pico as above.
