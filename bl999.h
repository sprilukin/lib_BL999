/*
 * bl999.h - library for Arduino to work with BL999 temperature/humidity sensor
 * See README file in this directory for more documentation
 *
 * Author: Sergey Prilukin sprilukin@gmail.com
 * $Id$
 */

#ifndef bl999_h
#define bl999_h

#include "Arduino.h"


// == PWM high and low pulse length ==

//high pulse length of the bit's divider
#define BL999_DIVIDER_PULSE_LENGTH 600

//length of the start bit low pulse
#define BL999_START_BIT_LENGTH 9000

//binary 1 low pulse length
#define BL999_BIT_1_LENGTH 3900

//binary 0 low puls length
#define BL999_BIT_0_LENGTH 1850


// == Thresholds for pulse length ==

//threshold for divider pulse
#define BL999_DIVIDER_THRESHOLD 100

//threshold for start bit pulse
#define BL999_START_BIT_THRESHOLD 1000

//threshold for 0 and 1 bit's pulse
#define BL999_REGULAR_BIT_THRESHOLD 500


// == Data array constants ==

//total amount of bits sent by BL999 sensor
#define BL999_DATA_BITS_AMOUNT 36

//36 bits are sent in packets by 4 bits (nibbles)
#define BL999_BITS_PER_PACKET 4

//totally we have 36/4 = 9 nibbles
#define BL999_DATA_ARRAY_SIZE BL999_DATA_BITS_AMOUNT / BL999_BITS_PER_PACKET

//In this struct result will be stored
typedef struct {
    byte channel : 2;
    byte powerUUID : 6;
    byte battery : 1;
    int temperature : 12;
    byte humidity : 8;
} BL999Info;

// Cant really do this as a real C++ class, since we need to have
// an ISR
extern "C" {

// API
extern void bl999_set_rx_pin(byte pin);

extern void bl999_rx_start();

extern void bl999_rx_stop();

extern void bl999_wait_rx();

extern boolean bl999_wait_rx_max(unsigned long milliseconds);

extern byte bl999_have_message();

extern boolean bl999_get_message(BL999Info& info);

// Private

extern void _bl999_rising();

extern void _bl999_falling();

extern boolean _bl999_isCheckSumMatch();

extern byte _bl999_getSensorChannel();

extern byte _bl999_getPowerUUID();

extern byte _bl999_getPowerStatus();

extern int _bl999_getTemperature();

extern byte _bl999_getHumidity();

extern void _bl999_fillDataArray(byte bitNumber, boolean isOne);

// Matcher for divider bit
extern boolean _bl999_matchDivider(int value);

// Matcher for start bit
extern boolean _bl999_matchStartBit(int value);

// Matcher for binary 1
extern boolean _bl999_matchOneBit(int value);

// Matcher for binary 0
extern boolean _bl999_matchZeroBit(int value);

//Whether pulse length value matches specified constant with specified threshold
extern boolean _bl999_match(int value, int mathConst, int threshold);

//set state to new value but only when condition is true
extern void _bl999_setState(byte st, boolean condition);
}

#endif