/*
 * Generate ACC signal for Viofo dashcams
 *
 * This source file is part of the follwoing repository:
 * http://www.github.com/microfarad-de/dashcam-controller
 *
 * Please visit:
 *   http://www.microfarad.de
 *   http://www.github.com/microfarad-de
 *
 * Copyright (C) 2025 Karim Hraibi (khraibi@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Version: 1.0.0
 * Date:    July 19, 2026
 */


#define VERSION_MAJOR 1  // Major version
#define VERSION_MINOR 0  // Minor version
#define VERSION_MAINT 0  // Maintenance version


#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>


/*
 * Pin assignment
 */
#define ACC_IN_PIN   4
#define ACC_OUT_PIN  2
#define LED_PIN     LED_BUILTIN  // 13


/*
 * Configuration parameters
 */
//#define SERIAL_DEBUG                 // Serial debug printing
#define SERIAL_BAUD              9600  // Serial communication baud rate
#define CLOCK_MULTIPLIER         1     // Clock multiplier for fast debugging
#define OFF_DELAY_S              60    // Power off delay in seconds after ACC input goes low
#define MIN_OFF_DURATION_S       15    // Minimum power off time in seconds before turning back on
#define ONE_SECOND               ((uint32_t)1000 / CLOCK_MULTIPLIER)  // 1 second duration in milliseconds


#ifdef SERIAL_DEBUG
  #define DEBUG(x) x
#else
  #define DEBUG(x)
#endif


/*
 * Main state machine state definitions
 */
typedef enum {
    STATE_OFF_HOLD_ENTRY = 0,
    STATE_OFF_HOLD,
    STATE_OFF_ENTRY,
    STATE_OFF,
    STATE_ON_ENTRY,
    STATE_ON,
    STATE_OFF_DELAY_ENTRY,
    STATE_OFF_DELAY
} State_e;


/*
 * State variables
 */
struct State_t {
    uint8_t accIn = LOW;     // ACC in level after debounce
} S;


/*
 * Function declarations
 */
void readInputPin (void);
void printVersion (uint8_t);


/*
 * Arduino initialization routine
 */
void setup (void)
{
    // Clear watchdog reset flag
    MCUSR &= ~(1 << WDRF);
    // Enable watchdog with 8 second timeout
    wdt_enable(WDTO_8S);

    // Make all pins an output and set them to low to reduce power,
    // except crystal pins.
    PORTB = 0;
    PORTC = 0;
    PORTD = 0;
    DDRB = 0x3F;
    DDRC = 0x3F;
    DDRD = 0xFF;

    pinMode(ACC_IN_PIN, INPUT);
    pinMode(ACC_OUT_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(ACC_OUT_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    // Disable ADC
    ADCSRA &= ~_BV(ADEN);
    power_adc_disable();

    // Disable analog comparator
    ACSR |= _BV(ACD);

    // Disable unused peripherals
    power_spi_disable();
    power_twi_disable();
    power_timer1_disable();
    power_timer2_disable();
    #ifndef SERIAL_DEBUG
        power_usart0_disable();
    #endif

    // Leave Timer0 enabled (Arduino core uses it)

    DEBUG(Serial.begin(SERIAL_BAUD));
    DEBUG(Serial.println(F("\r\n+ + +  D A S H C A M  C O N T R O L L E R  + + +\r\n")));
    DEBUG(printVersion(0));

}



/*
 * Arduino main loop
 */
void loop (void)
{
    static State_e state = STATE_OFF_ENTRY;
    static uint32_t stateTs = 0;

    set_sleep_mode(SLEEP_MODE_IDLE);

    noInterrupts();
    sleep_enable();
    interrupts();

    sleep_cpu();

    sleep_disable();

    uint32_t ts = millis();

    wdt_reset();
    readInputPin();

    switch (state) {

        /*
        * Force ACC output low and begin the minimum-low timer.
        */
        case STATE_OFF_HOLD_ENTRY:
            DEBUG(Serial.println(F("STATE_OFF_HOLD")));

            digitalWrite(ACC_OUT_PIN, LOW);
            digitalWrite(LED_PIN, LOW);

            stateTs = ts;
            state = STATE_OFF_HOLD;
            [[fallthrough]];

        /*
        * ACC output must remain low for the full hold duration.
        */
        case STATE_OFF_HOLD:
            if (ts - stateTs >= MIN_OFF_DURATION_S * ONE_SECOND) {
                state = STATE_OFF_ENTRY;
            }
            break;

        /*
        * Trace entry into the normal OFF state.
        */
        case STATE_OFF_ENTRY:
            DEBUG(Serial.println(F("STATE_OFF")));

            state = STATE_OFF;
            [[fallthrough]];

        /*
        * ACC output is low and may now be asserted immediately
        * when the debounced ACC input goes high.
        */
        case STATE_OFF:
            if (S.accIn == HIGH) {
                state = STATE_ON_ENTRY;
            }
            break;

        /*
        * Assert the simulated ACC output.
        */
        case STATE_ON_ENTRY:
            DEBUG(Serial.println(F("STATE_ON")));

            digitalWrite(ACC_OUT_PIN, HIGH);
            digitalWrite(LED_PIN, HIGH);

            state = STATE_ON;
            [[fallthrough]];

        /*
        * Normal ignition-on state.
        */
        case STATE_ON:
            if (S.accIn == LOW) {
                state = STATE_OFF_DELAY_ENTRY;
            }
            break;

        /*
        * ACC input has gone low, but the output remains high
        * while the power-off delay runs.
        */
        case STATE_OFF_DELAY_ENTRY:
            DEBUG(Serial.println(F("STATE_OFF_DELAY")));

            stateTs = ts;
            state = STATE_OFF_DELAY;
            [[fallthrough]];

        /*
        * Cancel shutdown if ACC input returns high.
        * Otherwise, enter the minimum-low state after the delay.
        */
        case STATE_OFF_DELAY:
            if (S.accIn == HIGH) {
                state = STATE_ON_ENTRY;
            }
            else if (ts - stateTs >= OFF_DELAY_S * ONE_SECOND) {
                state = STATE_OFF_HOLD_ENTRY;
            }
            break;

        /*
        * Fail-safe recovery.
        */
        default:
            digitalWrite(ACC_OUT_PIN, LOW);
            digitalWrite(LED_PIN, LOW);
            state = STATE_OFF_HOLD_ENTRY;
            break;
    }

}


/*
 * Read and debounce the input pin
 */
void readInputPin (void)
{
    static uint8_t avg = 0;
    const uint8_t samples = 8;
    uint8_t weight = digitalRead(ACC_IN_PIN) * 255;


    avg = (weight + (samples - 1) * avg) / samples;

    if (avg > 200) {
        S.accIn = HIGH;
    }
    else if (avg < 55) {
        S.accIn = LOW;
    }
}



/*
 * Print version info
 */
#define STRINGIFY(x) #x
#define QUOTE(x) STRINGIFY(x)
void printVersion (uint8_t indent)
{
    for (uint8_t i = 0; i < indent; i++) {
        Serial.print(" ");
    }
    Serial.println(F("V " QUOTE(VERSION_MAJOR) "." QUOTE(VERSION_MINOR) "." QUOTE(VERSION_MAINT)));
}
