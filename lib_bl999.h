/*
 * lib_bl999.h - library for Arduino to work with BL999 temperature/humidity sensor
 * See README file in this directory for more documentation
 *
 * Author: Sergey Prilukin sprilukin@gmail.com
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

// == Utility defines ==

//On some microcontrollers infinite loop cause watch dog reset,
//so introducing some small delay in loop
#define BL999_DELAY_ON_WAIT 10


//In this struct result will be stored
typedef struct {
    byte channel : 2; // up to 3 channels
    byte powerUUID : 6; //unique power state per current power on of the sensor
    byte battery : 1; // 0 - ok, 1- low
    int temperature : 12; // stored as temperature * 10: 217 = 21.7°
    byte humidity : 8; //1-99
} BL999Info;

// Cant really do this as a real C++ class, since we need to have
// an ISR
extern "C" {

// API

//set up digital pin which will be used to receive sensor signals
extern void bl999_set_rx_pin(byte pin);

//starts listening of the signals and read message from sensor(s)
extern void bl999_rx_start();

//stops listening for the signals
extern void bl999_rx_stop();

//blocks execution until message from sensor(s) will be received
extern void bl999_wait_rx();

//blocks execution until message from sensor(s) will be received
//but not more than for passed amount of milliseconds
extern boolean bl999_wait_rx_max(unsigned long milliseconds);

//returns true if whole message from sensor(s) was recieved
//NOTE: message will not be overridden with other messages
//until it will be read by the client using bl999_get_message
//NOTE2: this function does not take in account check sum
extern boolean bl999_have_message();

//if message was fully recieved (matches check sum or not)
//it will be written to info structure
//returns true if message received and check sum matches
//returns false otherwise
extern boolean bl999_get_message(BL999Info& info);

// Private

//rising ISR
extern void _bl999_rising();

//falling ISR
extern void _bl999_falling();

//calc check sum and compare with received one
extern boolean _bl999_isCheckSumMatch();

//get sensor channel from message
extern byte _bl999_getSensorChannel();

//get power UUID from message
extern byte _bl999_getPowerUUID();

//get power status form message
extern byte _bl999_getPowerStatus();

//get temperature from message
extern int _bl999_getTemperature();

//get humidity from message
extern byte _bl999_getHumidity();

//fill data array with next data bit
extern void _bl999_fillDataArray(byte bitNumber, byte value);

// Matcher for divider bit
extern boolean _bl999_matchDivider(unsigned long value);

// Matcher for start bit
extern boolean _bl999_matchStartBit(unsigned long value);

// Matcher for binary 1
extern boolean _bl999_matchOneBit(unsigned long int value);

// Matcher for binary 0
extern boolean _bl999_matchZeroBit(unsigned long int value);

//Whether pulse length value matches specified constant with specified threshold
extern boolean _bl999_match(unsigned long value, unsigned long mathConst, unsigned int threshold);

//set state to new value but only when condition is true
extern void _bl999_setState(byte st, boolean condition);
}

#endif