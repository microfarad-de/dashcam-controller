# Dashcam Controller

An Arduino-based controller for **VIOFO dashcams** that sits between the **VIOFO HK3 hardwire kit** and the dashcam to generate a clean ACC (ignition) signal.

The controller solves a firmware issue where rapid ignition ON/OFF transitions can prevent some VIOFO dashcams from detecting ignition loss correctly, causing the camera to remain powered indefinitely instead of entering parking mode.

## Features

* Designed for the VIOFO HK3 hardwire kit
* Debounces and delays the ACC signal
* Configurable ON and OFF delays
* Low power consumption (~1 mA standby)
* Uses AVR Idle sleep while keeping `millis()` running
* Simple finite state machine
* Optional serial debugging

## Circuit Diagram

<p align="center">
<img src="https://raw.githubusercontent.com/microfarad-de/dashcam-controller/master/doc/dashcam-controller-schematic.png" alt="Dashcam Controller Schematic"/>
</p>

The controller is installed inline between the **VIOFO HK3 hardwire kit** and the dashcam.

- The HK3 supplies regulated **5 V** to both the Arduino Pro Mini and the dashcam.
- **VBUS**, **D+**, **D−** and **GND** are passed directly from the HK3 to the dashcam.
- The **Mini-USB ID pin** carries the HK3 ACC signal.
- The Arduino monitors the HK3 ACC signal on **D4** through a 1 kΩ series resistor.
- A delayed and debounced ACC signal is generated on **D2**, which drives the dashcam's ID pin through a second 1 kΩ series resistor.

Only the ACC signal is modified; all other USB connections pass through unchanged. The HK3 continues to provide battery protection and the regulated 5 V supply.

## Operation

The controller continuously monitors the HK3 ACC signal and generates a delayed, debounced ACC output for the dashcam.

The firmware implements the following state machine:

```
OFF → ON_WAIT → ON
 ↑               │
 └── OFF_WAIT ←──┘
```

This guarantees reliable behaviour even if the ignition is cycled rapidly.

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

The controller behaviour can be adjusted by modifying the following parameters in the source code:

| Parameter | Description |
|-----------|-------------|
| `OFF_DELAY_S` | Time in seconds the dashcam remains in normal recording mode after the vehicle ignition is switched off. |
| `ON_DELAY_S` | Time in seconds before the dashcam is switched back to normal recording after the vehicle ignition is turned on. |
| `INPUT_DEBOUNCE_DELAY_MS` | Minimum time the ACC input must remain stable before a change is accepted, preventing false triggering due to ignition glitches or contact bounce. |
| `CLOCK_MULTIPLIER` | Scales all software timers for testing. A value greater than `1` speeds up long delays without modifying the configuration values. |
| `SERIAL_DEBUG` | Enables serial debug output showing firmware version and state transitions. |

During normal operation, `CLOCK_MULTIPLIER` should be left at `1`.

Enable `SERIAL_DEBUG` for diagnostic output during development.

## License

Licensed under the GNU General Public License v3.0 or later.
