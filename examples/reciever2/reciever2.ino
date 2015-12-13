#include <Arduino.h>
#include <bl999.h>

static BL999Info info;

void setup() {
    Serial.begin(115200);
    bl999_set_rx_pin(2);
    bl999_rx_start();
}

void loop() {
    if (bl999_wait_rx_max(1000)) {
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




