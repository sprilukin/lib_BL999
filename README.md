# lib_BL999

Arduino library for **BL999** remote Temperature/Humidity sensor

## Table of Contents

  1. [Sensor description](#sensor-description)
  1. [Protocol description](#protocol-description)
  1. [Structure of data bits in a message](#structure-of-data-bits-in-a-message)
  1. [Bits purpose](#bits-purpose)
  1. [Library usage](#library-usage)

## Sensor description
  * BL999 sensor working frequency is 433 Mhz
  * Max distance: 30 m
  * Temperature range -40 +50 Celsius
  * Humidity 1-99%

## Protocol description
  * BL999 uses PWM to send it's data.
  * BL999 message packet consists of `one` start bit and `36` value bits divided by PWM high pulses
  * BL999 repeats same message `4` times during one data sending session. 
  * Usual PWM oscillogram looks like:
  
```
  ___         ___     ___  ___
  | |         | |     | |  | |
  | |         | |     | |  | |
__| |_________| |_____| |__| |_ 

   |     |         |      |
    
   H1    L1        L2     L3
```
  
  * H1: PWM high pulse which is used as a divider of data bits. 
    It's length is `~600 micro seconds ± 100 microseconds`.
    All high pulses should looks like H1 in BL999 message packet.
  * L1: PWM low pulse which is used as a start bit marker. 
    It's length is `~9 millis ± 1 millis`
    It should always be present at the beginning of the message packet
  * L2: Data bit which represents binary `1`. It's length is `~3.9 ms ± 0.5 ms`
  * L3: Data bit which represents binary `0`. It's length is `~1.85 ms ± 0.5 ms`
  * Whole message consists of 36 data bits: 
    binary `1` or `0` (which looks like L2 or L3) which are divided by H1 high pulses
  
  
## Structure of data bits in a message

  * data bits in a message are grouped by nibbles (4 bit groups)
  * thus there are totally 9 nibbles in a message
 
```
 nibble:        T1    ||     T2    ||     T3    ||     T4    ||     T5
 bit:      A0|A1|A2|A3||B0|B1|B2|B3||C0|C1|C2|C3||D0|D1|D2|D3||E0|E1|E2|E3

 nibble:        T6    ||     T7    ||     T8    ||     T9
 bit:      F0|F1|F2|F3||G0|G1|G2|G3||H0|H1|H2|H3||I0|I1|I2|I3
```

## Bits purpose
  * **A0-A3,B2-B3**  - UUID which is randomly set once sensor is turned on
                and it is not changed until next power off

  * **B0-B1**  - Number of channel of the sensor: 
                 `01` - 1, `10` - 2, `11` - 3

  * **C0** - battery info: `0` - battery is ok, `1` - battery is low
  
  * **C1-C3** - unknown

  * **D0-F3** - temperature written backwards and multiplied by 10. For example:
         `1010|0011|0000` = 19.7° (Celsius) because `0000|1100|0101` = 197
         (note that not only nibbles written in reversed order, but bits in nibble also reversed)
         negative temperature is stored in [two's complement code](https://en.wikipedia.org/wiki/Two%27s_complement), 
         for example:
         `0010|1001|1111` = -10.8° (Celsius) because
         `01101100` (inverted all bits and added 1) = 108

  * **G0-H3** - humidity written backwards as `100 - HUMIDITY` in 
          [two's complement code](https://en.wikipedia.org/wiki/Two%27s_complement), 
          for example:
          `1110|1011` = 59% because `101001` (inverted all bits and add 1) = 41, and 100 - 41 = 59

  * **I0-I3** - 4 check sum which is calculated as a sum of less significant bits
            of check sum of the nibbles T1-T8 written backwards.
            for example let's look at the example message:
            `1011|1100|0100|0000|1111|0000|0010|1111|0010`
            sum of the nibbles T1-T8 where each bit written backwards = 110100,
            lets take last 4 bits and wrote them backward:
            0010 which is equal to T9 nibble
            
## Library usage

  * This library is intended to be used with some 433 Mhz receiver.
    Please read datasheet for your receiver about how to use it with Arduino
  * In the example below signal pin of the receiver 
    is expected to be attached to Arduino digital pin 2.
    Please note that only digital pins which supports ISR should be used.
    See [Digital Pins suitable for ISR on Arduino](https://www.arduino.cc/en/Reference/AttachInterrupt).
    Sketch infinetely wait for message from any BL999 transmitter
    and writes it to serial port
  
```C
#include <lib_bl999.h>

static BL999Info info;

void setup() {
    Serial.begin(115200);
    
    //Set pin
    bl999_set_rx_pin(2);
    
    //Start ISR machinery
    bl999_rx_start();
}

void loop() {
    //wait infinetly until data will be received. 
    //Either correct or not. 
    //CRC does not checked at this moment
    bl999_wait_rx();
    
    //If data received and CRC correct - print to serial
    if (bl999_get_message(info)) {
        Serial.println("====== Got message: ");
        Serial.println(info.channel);
        Serial.println(info.powerUUID);
        Serial.println(info.battery);
        Serial.println(info.temperature);
        Serial.println(info.humidity);
    }
}
```
