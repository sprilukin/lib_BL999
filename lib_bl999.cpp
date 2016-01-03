/*
 * lib_bl999.cpp - library for Arduino to work with BL999 temperature/humidity sensor
 * See README file in this directory for more documentation
 *
 * Author: Sergey Prilukin sprilukin@gmail.com
 */

#include <Arduino.h>
#include "lib_bl999.h"

static volatile unsigned long bl999_pwm_high_length = 0;
static volatile unsigned long bl999_pwm_low_length = 0;
static volatile unsigned long bl999_prev_time_rising = 0;
static volatile unsigned long bl999_prev_time_falling = 0;
static volatile byte bl999_state = 0;
static byte bl999_data[BL999_DATA_ARRAY_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static byte bl999_pin = 2;
static volatile boolean bl999_active = false;
static volatile boolean bl999_message_ready = false;

extern "C" {

//=====================
//Public API
//=====================

extern void bl999_set_rx_pin(byte pin) {
    bl999_rx_stop();
    bl999_pin = pin;
}

extern void bl999_rx_start() {
    if (!bl999_active) {
        attachInterrupt(digitalPinToInterrupt(bl999_pin), _bl999_rising, RISING);
        bl999_state = 0;
        bl999_active = true;
    }
}

extern void bl999_rx_stop() {
    if (bl999_active) {
        detachInterrupt(digitalPinToInterrupt(bl999_pin));
        bl999_active = false;
        bl999_state = 0;
    }
}

extern void bl999_wait_rx() {
//    while (bl999_active && !bl999_message_ready)
//        ;
    byte state = bl999_state;
    while (bl999_state == state)
        ;
}

/*extern boolean bl999_wait_rx_max(unsigned long milliseconds) {
    unsigned long start = millis();

    while (bl999_active && !bl999_message_ready && ((millis() - start) < milliseconds))
        ;

    return bl999_message_ready;
}*/

extern byte bl999_have_message() {
    //return bl999_message_ready;
    return true;
}

extern boolean bl999_get_message(BL999Info& info) {

    // Message available?
    /*if (!bl999_message_ready) {
        return false;
    }

    info.channel = _bl999_getSensorChannel();
    info.powerUUID = _bl999_getPowerUUID();
    info.battery = _bl999_getPowerStatus();
    info.temperature = _bl999_getTemperature();
    info.humidity = _bl999_getHumidity();

    byte checkSumMatches = _bl999_isCheckSumMatch();

    bl999_state = 0;
    bl999_message_ready = false;

    return checkSumMatches;*/

    info.channel = bl999_state;
    return true;
}

//===============
//Private
//===============

extern void _bl999_rising() {
    attachInterrupt(digitalPinToInterrupt(bl999_pin), _bl999_falling, FALLING);
    bl999_state = !bl999_state;

    //Do not rewrite last received but unread message
    /*if (!bl999_message_ready) {

        //will be used in falling interrupt
        bl999_prev_time_rising = micros();

        bl999_pwm_low_length = micros() - bl999_prev_time_falling;

        if (bl999_state % 2 == 0) {
            //Clear state to 0 since we are now in ivalid state
            _bl999_setState(0, true);
        } else {
            if (bl999_state == 1) {
                //We expect start bit to match current LOW pulse
                _bl999_setState(2, _bl999_matchStartBit(bl999_pwm_low_length));
                return;
            }

            byte newState = bl999_state + 1;
            boolean matchOne = _bl999_matchOneBit(bl999_pwm_low_length);
            boolean matchZero = _bl999_matchZeroBit(bl999_pwm_low_length);

            //We expect value bit (0 or 1) to match current LOW pulse
            _bl999_setState(newState, matchOne || matchZero);

            if (bl999_state == newState) {
                //Ok, we've read next value bit, lets calc it's number
                int bitNumber = (bl999_state - 1) / 2 - 1;

                //fill data array with this next value bit
                _bl999_fillDataArray(bitNumber, (byte)matchOne);

                if (bitNumber == BL999_DATA_BITS_AMOUNT - 1) {
                    //This was the last value bit

                    //All is done we can return the whole message to the client
                    //Do not calc check sum here.
                    //It will be calculated at message reading stage
                    //if (_bl999_isCheckSumMatch()) {
                        bl999_message_ready = true;
                    //}

                    //clear state to zero since nothing more we can do
                    _bl999_setState(0, true);
                    bl999_prev_time_rising = 0;
                    bl999_prev_time_falling = 0;
                    bl999_pwm_high_length = 0;
                    bl999_pwm_low_length = 0;
                }
            }
        }
    }*/
}

extern void _bl999_falling() {
    attachInterrupt(digitalPinToInterrupt(bl999_pin), _bl999_rising, RISING);
    bl999_state = !bl999_state;

    //Do not rewrite last received but unread message
    /*if (!bl999_message_ready) {
        //remember current time
        //it will be used in rising function
        bl999_prev_time_falling = micros();

        //calc length of last HIGH pulse
        bl999_pwm_high_length = micros() - bl999_prev_time_rising;

        if (bl999_state % 2 == 0) {
            //We only can proceed if state is even: 0, 2, 4 - after this state
            //we always expect divider HIGH pulse to happen
            _bl999_setState(bl999_state + 1, _bl999_matchDivider(bl999_pwm_high_length));
        } else {
            //Clear state to zero since it happens that we are in invalid state
            //at this point
            _bl999_setState(0, true);
        }
    }*/
}

/*extern boolean _bl999_isCheckSumMatch() {

    //Sum first 8 nibbles
    int sum = 0;
    for (byte i = 0; i < BL999_DATA_ARRAY_SIZE - 1; i++) {
        sum += bl999_data[i];
    }

    //clear higher bits
    sum &= 15;

    //returns true if calculated check sum matches received
    return sum == bl999_data[BL999_DATA_ARRAY_SIZE - 1];
}

extern byte _bl999_getSensorChannel() {
    //Channel is bits B0, B1 in T2 nibble
    //Since we store T2 in reversed order
    //we have to get 3,4 bits ot data[1] and reverse them
    return ((bl999_data[1] & 1) << 1) | ((bl999_data[1] & 2) >> 1);
}

extern byte _bl999_getPowerUUID() {
    //we do not reverse power uuid since value is still uniq
    return (bl999_data[0] << 2) | ((bl999_data[1] & 12) >> 2);
}

extern byte _bl999_getPowerStatus() {
    //Power or battery status is C0 bit in T3 nibble
    //since in data array we wrote T3 in reversed order
    //it will be the first bit in data[2]
    return bl999_data[2] & 1;
}

extern int _bl999_getTemperature() {
    int temperature = 0;

    //Temperature is stored in T4,T5,T6 nibbles
    //lowest nibble - first
    //since we already reversed bits order in these nibbles
    //all we have to do is to reverse nibbles order
    temperature = (((int)bl999_data[5] << 8)
                   | ((int)bl999_data[4] << 4)
                   | (int)bl999_data[3]);

    if ((bl999_data[5] & 1) == 1) {
        //negative number, use two's compliment conversion
        temperature = ~temperature + 1;

        //clear higher bits and convert to negative
        temperature = -1 * (temperature & 4095);
    }

    return temperature;
}

extern byte _bl999_getHumidity() {

    //Humidity is stored in nibbles T7,T8
    //since bits in these nibbles are already in reversed order
    //we just have to get number by reversing nibbles orderßßß
    int humidity = ((int)bl999_data[7] << 4) | (int)bl999_data[6];

    //negative number, use two's compliment conversion
    humidity = ~humidity + 1;

    //humidity is stored as 100 - humidity
    return 100 - (byte)humidity;
}

extern void _bl999_fillDataArray(byte bitNumber, byte value) {
    byte dataArrayIndex = bitNumber / BL999_BITS_PER_PACKET;
    byte bitInNibble = bitNumber % 4;

    if (bitInNibble == 0) {
        // if it's the first bit in nibble -
        // clear nibble since it could be filled with random data at this time
        bl999_data[dataArrayIndex] = 0;
    }

    //Write all nibbles in reversed order
    //so it will be easier to do calculations later
    bl999_data[dataArrayIndex] |= (value << bitInNibble);
}

// Matcher for divider bit
extern boolean _bl999_matchDivider(int value) {
    return _bl999_match(value, BL999_DIVIDER_PULSE_LENGTH, BL999_DIVIDER_THRESHOLD);
}

// Matcher for start bit
extern boolean _bl999_matchStartBit(int value) {
    return _bl999_match(value, BL999_START_BIT_LENGTH, BL999_START_BIT_THRESHOLD);
}

// Matcher for binary 1
extern boolean _bl999_matchOneBit(int value) {
    return _bl999_match(value, BL999_BIT_1_LENGTH, BL999_REGULAR_BIT_THRESHOLD);
}

// Matcher for binary 0
extern boolean _bl999_matchZeroBit(int value) {
    return _bl999_match(value, BL999_BIT_0_LENGTH, BL999_REGULAR_BIT_THRESHOLD);
}

//Whether pulse length value matches specified constant with specified threshold
extern boolean _bl999_match(int value, int mathConst, int threshold) {
    return value > mathConst - threshold && value < mathConst + threshold;
}

//set state to new value but only when condition is true
extern void _bl999_setState(byte st, boolean condition) {
    bl999_state = condition ? st : 0;
}*/
}