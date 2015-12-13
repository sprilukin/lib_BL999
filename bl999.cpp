/*
 * bl999.h - library for Arduino to work with BL999 temperature/humidity sensor
 * See README file in this directory for more documentation
 *
 * Author: Sergey Prilukin sprilukin@gmail.com
 * $Id$
 */

#include <Arduino.h>
#include "bl999.h"

static volatile unsigned int bl999_pwm_high_length = 0;
static volatile unsigned int bl999_pwm_low_length = 0;
static volatile unsigned int bl999_prev_time_rising = 0;
static volatile unsigned int bl999_prev_time_falling = 0;
static volatile byte bl999_state = 0;
static byte bl999_data[DATA_ARRAY_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static byte bl999_pin = 2;
static boolean bl999_active = false;
static boolean bl999_message_ready = false;

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
        bl999_active = true;
    }
}

extern void bl999_rx_stop() {
    if (bl999_active) {
        detachInterrupt(digitalPinToInterrupt(bl999_pin));
        bl999_active = false;
    }
}

extern void bl999_wait_rx() {
    while (bl999_active && !bl999_message_ready)
        ;
}

extern byte bl999_wait_rx_max(unsigned long milliseconds) {
    unsigned long start = millis();

    while (bl999_active && !bl999_message_ready && ((millis() - start) < milliseconds))
        ;
    return bl999_message_ready;
}

extern byte bl999_have_message() {
    return bl999_message_ready;
}

extern boolean bl999_get_message(byte *buf) {
    uint8_t rxlen;

    // Message available?
    if (!bl999_message_ready)
        return false;

    memcpy(buf, bl999_data, DATA_ARRAY_SIZE);

    bl999_state = 0;
    bl999_message_ready = false;

    return true;
}

//===============
//Private
//===============

extern void _bl999_rising() {
    attachInterrupt(digitalPinToInterrupt(bl999_pin), _bl999_falling, FALLING);
}

extern void _bl999_falling() {
    attachInterrupt(digitalPinToInterrupt(bl999_pin), _bl999_rising, RISING);

    if (!bl999_message_ready) {
        if (bl999_state < DATA_ARRAY_SIZE) {
            bl999_data[bl999_state] = 0xF;
            bl999_state += 1;
        }

        if (bl999_state == DATA_ARRAY_SIZE) {
            bl999_message_ready = true;
        }
    }
}

}