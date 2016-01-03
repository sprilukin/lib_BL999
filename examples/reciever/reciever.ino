/*
 * An example of lib_bl999 library - library for Arduino to work with
 * BL999 temperature/humidity sensor
 * See README file in this directory for more documentation
 *
 * Author: Sergey Prilukin sprilukin@gmail.com
 */

#include <lib_bl999.h>

static BL999Info info;

void setup() {
    Serial.begin(115200);

    //set digital pin to read info from
    bl999_set_rx_pin(4);

    //start reading data from sensor
    bl999_rx_start();
}

void loop() {

    //blocks until message will not be read
    if (bl999_have_message()) {
        //read message to info and if check sum correct - output it to serial port
        if (bl999_get_message(info)) {
            Serial.println("====== Got message: ");
            Serial.println(info.channel);
            Serial.println(info.powerUUID);
            Serial.println(info.battery);
            Serial.println(info.temperature);
            Serial.println(info.humidity);
        }
    }

    delay(1000);
}