# Dashcam Controller

An Arduino-based controller for **VIOFO dashcams** that sits between the **VIOFO HK3 hardwire kit** and the dashcam to generate a clean ACC (ignition) signal.

The controller solves a firmware issue where rapid ignition ON/OFF transitions can prevent some VIOFO dashcams from detecting ignition loss correctly, causing the camera to remain powered indefinitely instead of entering parking mode.

## Features

* Designed for the VIOFO HK3 hardwire kit
* Debounces and delays the ACC signal
* Configurable ON and OFF delays
* Low power consumption (~2 ot ~5 mA standby)
* Uses AVR Idle sleep while keeping `millis()` running
* Simple finite state machine
* Optional serial debugging

## Prerequisites

* ATmega328P based Arduino Pro Mini, Arduino Nano or similar model
* The code uses the watchdog timer, therefore the custom bootloader with watchdog support must be installed from: https://github.com/microfarad-de/bootloader

## Circuit Diagram

<p align="center">
<img src="https://raw.githubusercontent.com/microfarad-de/dashcam-controller/master/doc/dashcam-controller-schematic.png" alt="Dashcam Controller Schematic"/>
</p>

The controller is installed inline between the **VIOFO HK3 hardwire kit** and the dashcam.

- The HK3 supplies regulated **5 V** to both the Arduino Pro Mini and the dashcam.
- **VBUS** and **GND** are passed directly from the HK3 to the dashcam.
- The **Mini-USB ID pin** carries the HK3 ACC signal.
- The Arduino monitors the HK3 ACC signal on **D4** through a 1 kΩ series resistor.
- A delayed and debounced ACC signal is generated on **D2**, which drives the dashcam's ID pin through a second 1 kΩ series resistor.

Only the ACC signal is modified; all other USB connections pass through unchanged. The HK3 continues to provide battery protection and the regulated 5 V supply.

## Operation

The controller continuously monitors the HK3 ACC signal and generates a delayed, debounced ACC output for the dashcam.

The firmware implements the following state machine:

## Operation

The controller uses four functional states:

<p align="center">
<img src="https://raw.githubusercontent.com/microfarad-de/dashcam-controller/master/doc/state-machine.png" alt="State Machine"/>
</p>


### OFF

The dashcam ACC output is held **LOW**, indicating that the ignition is off. The controller remains in this state until the debounced ACC input becomes **HIGH**, at which point the dashcam is immediately switched on.

### ON

The dashcam ACC output is held **HIGH**, indicating that the ignition is on. When the debounced ACC input becomes **LOW**, the controller starts the configurable power-off delay by transitioning to **OFF_DELAY**.

### OFF_DELAY

The dashcam ACC output remains **HIGH** while the power-off delay (`OFF_DELAY_S`) is running. This prevents the dashcam from switching to parking mode during short ignition interruptions. Once the delay expires, the ACC output is switched **LOW** and the controller enters **OFF_HOLD**.

### OFF_HOLD

The dashcam ACC output is held **LOW** for at least `MIN_OFF_DURATION_S`, regardless of the ACC input. This guarantees that the dashcam always observes a complete ACC-OFF pulse before normal operation resumes. When the hold time expires, the controller returns to **OFF**.

## Power Saving

Unused peripherals are disabled to reduce standby current:

* ADC
* Analog comparator
* SPI
* TWI (I²C)
* Timer1
* Timer2
* UART (unless serial debugging is enabled)

Timer0 remains active so the standard Arduino timing functions continue to work.

## Configuration

The controller behaviour can be adjusted by modifying the following parameters in the source code.

| Parameter | Description |
|-----------|-------------|
| `OFF_DELAY_S` | Time in seconds the simulated ACC output remains HIGH after the vehicle ACC input goes LOW before the dashcam is switched off. |
| `MIN_OFF_DURATION_S` | Minimum time in seconds the simulated ACC output remains LOW before it may be asserted again. This guarantees the dashcam always observes a complete ACC-OFF period. |
| `CLOCK_MULTIPLIER` | Accelerates the ON/OFF timing during development by scaling the configured delays. Leave set to `1` for normal operation. |
| `SERIAL_DEBUG` | Enables serial debug output showing the firmware version and state transitions. |

## Gallery


 <p align="center">
 <img src="https://raw.githubusercontent.com/microfarad-de/dashcam-controller/master/doc/perspective-1.png" alt="drawing" width="600"/>
 </p>

 <p align="center">
 <img src="https://raw.githubusercontent.com/microfarad-de/dashcam-controller/master/doc/perspective-2.png" alt="drawing" width="600"/>
 </p>

 <p align="center">
 <img src="https://raw.githubusercontent.com/microfarad-de/dashcam-controller/master/doc/perspective-3.png" alt="drawing" width="600"/>
 </p>
