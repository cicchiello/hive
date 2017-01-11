/*********************************************************
JFC START COMMENT: 

This entire directory was copied from the Adafruit Arduino package install at:

     ~/.arduino15/packages/adafruit/hardware/samd/1.0.9/bootloaders/feather
     
Then some minor modifications were made so that I could determine progress (make LED
bootload-mode flashing controllable so I could be certain I had actually uploaded code).

Then, at a later time (1/10/17), I needed it again, and found a slightly updated
source at:
     ~/.arduino15/packages/adafruit/hardware/samd/1.0.13/bootloaders/feather
Which I proceeded to manually merge into this directory.  Most of the differences
appeared to be directly related to handshaking/timing for the samd21g18 -- so I took all of them.


Wiring:
   - RPi2 GPIO header pin 6 -> GND
   - RPi2 GPIO header pin 12 -> NRESET (pin 1 on Adafruit Feather M0+ BLE)
   - RPi2 GPIO header pin 18 -> swdio (tiny pad on bottom of module)
   - RPi2 GPIO header pin 22 -> swclk (tiny pad on bottom of module)
   

Note1: I would have prefered to use a github repo, but couldn't find it (unlike most of Adafruit's support code),
therefore I won't benefit from any improvements made by the original authors and I also need to keep it in it's
entirety in our own repo.

Note2: Though there are similarities, it is *not* identical to the Arduino Zero bootloader.  That module has
an extra chip on it to provide EDBG support and it appears to provide some sort of marshalling interface for
the low-level SWD programming interface.

JFC END COMMENT: 
*********************************************************/


------------------------------------------------
# Arduino Zero Bootloader

## 1- Prerequisites

The project build is based on Makefile system.
Makefile is present at project root and try to handle multi-platform cases.

Multi-plaform GCC is provided by ARM here: https://launchpad.net/gcc-arm-embedded/+download

Atmel Studio contains both make and ARM GCC toolchain. You don't need to install them in this specific use case.

### Windows

* Native command line
Make binary can be obtained here: http://gnuwin32.sourceforge.net/packages/make.htm

* Cygwin/MSys/MSys2/Babun/etc...
It is available natively in all distributions.

* Atmel Studio
An Atmel Studio **7** Makefile-based project is present at project root, just open samd21_sam_ba.atsln file in AS7.

### Linux

Make is usually available by default.

### OS X

Make is available through XCode package.


## 2- Selecting available SAM-BA interfaces

By default both USB and UART are made available, but this parameter can be modified in sam_ba_monitor.h, line 31:

Set the define SAM_BA_INTERFACE to
* SAM_BA_UART_ONLY for only UART interface
* SAM_BA_USBCDC_ONLY for only USB CDC interface
* SAM_BA_BOTH_INTERFACES for enabling both the interfaces

## 3- Behaviour

This bootloader implements the double-tap on Reset button.
By quickly pressing this button two times, the board will reset and stay in bootloader, waiting for communication on either USB or USART.

The USB port in use is the USB Native port, close to the Reset button.
The USART in use is the one available on pins D0/D1, labelled respectively RX/TX. Communication parameters are a baudrate at 115200, 8bits of data, no parity and 1 stop bit (8N1).

## 4- Description

**Pinmap**

The following pins are used by the program :
PA25 : input/output (USB DP)
PA24 : input/output (USB DM)
PA11 : input (USART RX)
PA10 : output (USART TX)

The application board shall avoid driving the PA25, PA24, PB23 and PB22 signals while the boot program is running (after a POR for example).

**Clock system**

CPU runs at 48MHz from Generic Clock Generator 0 on DFLL48M.

Generic Clock Generator 1 is using external 32kHz oscillator and is the source of DFLL48M.

USB and USART are using Generic Clock Generator 0 also.

**Memory Mapping**

Bootloader code will be located at 0x0 and executed before any applicative code.

Applications compiled to be executed along with the bootloader will start at 0x2000 (see linker script bootloader_samd21x18.ld).

Before jumping to the application, the bootloader changes the VTOR register to use the interrupt vectors of the application @0x2000.<- not required as application code is taking care of this.
