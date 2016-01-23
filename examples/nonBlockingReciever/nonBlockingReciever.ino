/*
 * An example of lib_bl999 library - library for Arduino to work with
 * BL999 temperature/humidity sensor
 * See https://github.com/sprilukin/lib_BL999 for more documentation
 *
 * This is non-blocking example.
 * Since library uses interrupts
 * it's enough just to check time-to time whether message is present.
 *
 * Author: Sergey Prilukin sprilukin@gmail.com
 */

#include <lib_bl999.h>

static BL999Info info;

void setup() {
    Serial.begin(115200);

    //set digital pin to read info from
    bl999_set_rx_pin(2);

    //start reading data from sensor
    bl999_rx_start();
}

void loop() {

    //read message to info and if check sum correct - outputs it to the serial port
    //does not block main loop
    if (bl999_have_message() && bl999_get_message(info)) {
        Serial.println("====== Got message from BL999 sensor: ");
        Serial.println("Channel: " + String(info.channel));
        Serial.println("PowerUUID: " + String(info.powerUUID));
        Serial.println("Battery is " + String(info.battery == 0 ? "Ok" : "Low"));
        Serial.println("Temperature: " + String(info.temperature / 10.0));
        Serial.println("Humidity: " + String(info.humidity) + "%");
    }
}
