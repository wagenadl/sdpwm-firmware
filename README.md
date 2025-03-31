# Firmware for the SDPWM paper

## Introduction

This is the firmware accompanying the paper “Improving PWM output from
common MCUs with a Sigma-Delta stage”, now available as a
[preprint](https://www.techrxiv.org/users/907341/articles/1281376-improving-pwm-output-from-common-mcus-with-a-software-sigma-delta-stage).

## Circuit diagram

For testing purposes, the circuit is simple enough that it can be built on a breadboard.

![Breadboard](https://github.com/wagenadl/sdpwm-firmware/blob/main/breadboard.png?raw=true)

(Image created with [Fritzing](https://fritzing.org/) and [GIMP](https://www.gimp.org/).)

The circuit comprises a Raspberry Pi [Pico](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html) 
microcontroller (left), the filter described in the paper (middle) and a 5V → ±12V DC-DC converter (right). 
The results in the paper were obtained with a Texas Instruments [LM833N](https://www.ti.com/product/LM833-N/part-details/LM833N/NOPB) opamp and a 
Murata [CRV1D0512SC](https://www.murata.com/en-us/products/productdetail?partno=CRV1D0512SC) DC-DC converter, 
but similarly specified devices may be substituted. Be aware that similar devices may have different pinouts. 
We used a v.1 Pico. The firmware should also work with Pico W, Pico 2, or Pico 2 W boards without modification.

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

## Using the firmware

The firmware opens a serial port on your host computer. You can communicate with that using the Arduino “Serial Monitor” or using the “pyserial” module. When first connected, the firmware announces itself as

  sdpwm 1.0 - Daniel Wagenaar 2025

After that, you can send commands to it. It understands the following commands:

* sdpwm — switch to ΣΔPWM mode
* sdm — switch to ΣΔM mode
* pwm — switch to PWM mode
* logk *k* — set the modulation frequency to 125 MHz / (1<<*k*)
* per *n* — set the sample period to *n* over the modulation frequency
* over *m* — set oversampling to 1<<*m*
* pause — pause operations
* go — resume operations
* bin — enter binary mode

Useful values for *k* are 4 through 11. Useful values for *m* are 0, 1, 2. Not all combinations of *k*, *m*, and *n* work.
The “pause” and “go” commands must strictly come in pairs.
  
If you send a number (between −32767 and +32767, in decimal ascii representation) as a command, that value is immediately used to drive the output. To queue multiple numbers to be output at the sampling rate implied by “logk”, “per”, and “over”, you can send “pause”, then a sequence of numbers, then “go”. The device has a buffer to hold up to 65,000 samples. When the buffer underruns, the last sample is repeated until more are provided. When the buffer is full, the USB interface blocks until more space appears. If the system is “paused” while the buffer becomes full, there is no point in waiting, so the system resets instead.

To send longer sequences, it may be necessary to switch to “binary mode”, as the ascii-based interface is not very fast. In binary mode, pairs of bytes are interpreted as signed 16-bit integers (low-byte first). To return to ascii mode, send the number 0x8080. 

## Code examples

Code examples for interacting with the firmware from Python are provided in the “examples” folder. As written, these examples assume you have a National Instruments DAQ card to capture the output. Alternatively, you can remove all references to “nidaqmx” from the code and use an oscilloscope to observe the output.
