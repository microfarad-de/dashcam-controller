/*
 * Generate ACC signal for viofo dashcams
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
 * Date:    July 18, 2026
 */


#define VERSION_MAJOR 1  // Major version
#define VERSION_MINOR 0  // Minor version
#define VERSION_MAINT 0  // Maintenance version


#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>


/*
 * Pin assignment
 */
#define ACC_IN_PIN   4
#define ACC_OUT_PIN  2
#define LED_PIN     LED_BUILTIN  // 13


/*
 * Configuration parameters
 */
#define SERIAL_DEBUG                   // Serial debug printing
#define SERIAL_BAUD              9600  // Serial communication baud rate
#define CLOCK_MULTIPLIER         1     // Clock multiplier for fast debugging
#define OFF_DELAY_S              60    // Power off delay in seconds after ACC input goes low
#define ON_DELAY_S               10    // Power on delay in saconds after ACC input goes high
#define INPUT_DEBOUNCE_DELAY_MS  1000  // Input debounce time delay in milliseconds
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
    STATE_OFF_ENTRY      = 0,
    STATE_OFF            = 1,
    STATE_ON_WAIT_ENTRY  = 2,
    STATE_ON_WAIT        = 3,
    STATE_ON_ENTRY       = 4,
    STATE_ON             = 5,
    STATE_OFF_WAIT_ENTRY = 6,
    STATE_OFF_WAIT       = 7
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
    static uint32_t onTs = 0;
    static uint32_t offTs = 0;

    set_sleep_mode(SLEEP_MODE_IDLE);

    noInterrupts();
    sleep_enable();
    interrupts();

    sleep_cpu();

    sleep_disable();

    uint32_t ts = millis();

    readInputPin();

    switch (state) {

        case STATE_OFF_ENTRY:
            DEBUG(Serial.println(F("STATE_OFF")));
            state = STATE_OFF;
        case STATE_OFF:
            if (HIGH == S.accIn) {
                state = STATE_ON_WAIT_ENTRY;
            }
            break;

        case STATE_ON_WAIT_ENTRY:
            DEBUG(Serial.println(F("STATE_ON_WAIT")));
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(ACC_OUT_PIN, HIGH);
            offTs = ts;
            state = STATE_ON_WAIT;
        case STATE_ON_WAIT:
            if (ts - offTs > OFF_DELAY_S * ONE_SECOND) {
                state = STATE_ON_ENTRY;
            }
            break;

        case STATE_ON_ENTRY:
            DEBUG(Serial.println(F("STATE_ON")));
            state = STATE_ON;
        case STATE_ON:
            if (LOW == S.accIn) {
                state = STATE_OFF_WAIT_ENTRY;
            }
            break;

        case STATE_OFF_WAIT_ENTRY:
            DEBUG(Serial.println(F("STATE_OFF_WAIT")));
            digitalWrite(LED_PIN, LOW);
            digitalWrite(ACC_OUT_PIN, LOW);
            onTs = ts;
            state = STATE_OFF_WAIT;
        case STATE_OFF_WAIT:
            if (ts - onTs > ON_DELAY_S * ONE_SECOND) {
                state = STATE_OFF_ENTRY;
            }
            break;

        default:
            break;

    }

}


/*
 * Read and debounce the input pin
 */
void readInputPin (void)
{
  static uint32_t inputTs = 0;
  uint32_t ts = millis();


  if (HIGH == S.accIn) {
    if (HIGH == digitalRead(ACC_IN_PIN)) inputTs = ts;
    if (ts - inputTs > INPUT_DEBOUNCE_DELAY_MS) {
      S.accIn = LOW;
    }
  }
  else {
    if (LOW == digitalRead(ACC_IN_PIN)) inputTs = ts;
    if (ts - inputTs > INPUT_DEBOUNCE_DELAY_MS) {
      S.accIn = HIGH;
    }
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
