#include <Arduino.h>
#include <bl999.h>

static byte data[DATA_ARRAY_SIZE];

void setup() {
    bl999_set_rx_pin(2);
    Serial.begin(115200);
}

void loop() {
    bl999_rx_start();
    bl999_wait_rx();
    if (bl999_get_message(data)) {
        Serial.print("Got message: ");
        Serial.print(data[0]);
    }
}




