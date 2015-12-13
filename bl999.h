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

//PWM high and low pulse length
#define DIVIDER_PULSE_LENGTH 600 //high pulse length of the bit's divider
#define START_BIT_LENGTH 9000 //length of the start bit low pulse
#define BIT_1_LENGTH 3900 //binary 1 low pulse length
#define BIT_0_LENGTH 1850 //binary 0 low puls length

//Thresholds for pulse length
#define DIVIDER_THRESHOLD 100 //threshold for divider pulse
#define START_BIT_THRESHOLD 1000 //threshold for start bit pulse
#define REGULAR_BIT_THRESHOLD 500 //threshold for 0 and 1 bit's pulse

#define DATA_BITS_AMOUNT 36  //total amount of bits sent by BL999 sensor
#define BITS_PER_PACKET 4 //36 bits are sent in packets by 4 bits (tetrads)
#define DATA_ARRAY_SIZE DATA_BITS_AMOUNT / BITS_PER_PACKET //totally we have 36/4 = 9 tetrads

// Cant really do this as a real C++ class, since we need to have
// an ISR
extern "C" {

extern void bl999_set_rx_pin(byte pin);

extern void bl999_rx_start();

extern void bl999_rx_stop();

extern void bl999_wait_rx();

extern byte bl999_wait_rx_max(unsigned long milliseconds);

extern byte bl999_have_message();

extern boolean bl999_get_message(byte* buf);

// Private

extern void _bl999_rising();

extern void _bl999_falling();
}

#endif