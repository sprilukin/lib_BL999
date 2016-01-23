# lib_BL999

Arduino library for **BL999** remote Temperature/Humidity sensor<br/>
It supports both **BL999** sensor types: with humidity and temperature-only

## Table of Contents

  1. [Sensor description](#sensor-description)
  1. [Protocol description](#protocol-description)
  1. [Structure of data bits in a message](#structure-of-data-bits-in-a-message)
  1. [Bits purpose](#bits-purpose)
  1. [Library usage](#library-usage)
  1. [API description](#api-description)

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

    //set digital pin to read info from
    bl999_set_rx_pin(2);

    //start reading data from sensor
    bl999_rx_start();
}

void loop() {

    //waits infinitely until message will be received.
    //Either correct or not.
    //check sum does not checked at this moment
    bl999_wait_rx();

    //read message to info and if check sum correct - outputs it to the serial port
    if (bl999_get_message(info)) {
        Serial.println("====== Got message from BL999 sensor: ");
        Serial.println("Channel: " + info.channel);
        Serial.println("PowerUUID: " + info.powerUUID);
        Serial.println("Battery is " + (info.battery == 0 ? "Ok" : "Low"));
        Serial.println("Temperature: " + String(info.temperature / 10.0));
        Serial.println("Humidity: " + info.humidity + "%");
    }
}
```

  * The next example is the same except that in this example main loop does not blocked at all
  
```C
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
        Serial.println("Channel: " + info.channel);
        Serial.println("PowerUUID: " + info.powerUUID);
        Serial.println("Battery is " + String(info.battery == 0 ? "Ok" : "Low"));
        Serial.println("Temperature: " + String(info.temperature / 10.0));
        Serial.println("Humidity: " + String(info.humidity) + "%");
    }
}
```  

## API description

  * **BL999Info** - stucture which holds sensor information like 
                        channel, temperature, humidity and power UUID
   ```
   typedef struct {
       byte channel : 2;     // up to 3 channels
       byte powerUUID : 6;   //unique power state per current power on of the sensor
       byte battery : 1;     // 0 - ok, 1- low
       int temperature : 12; // temperature * 10, for example 217 means 21.7°C
       byte humidity : 8;    //1-99 humidity in %
   } BL999Info;
   ```
   
  * **void bl999_set_rx_pin(byte pin)** - set up digital pin which will be used to 
    receive sensor signals. Note: digital pin should support ISR, 
    see datasheet for your microcontroller to find pins which supports interrupts 
  * **void bl999_rx_start()** - starts listening for messages from BL999 sensor(s)
  * **void bl999_rx_stop()** - stops listening for the messages from sensor(s) 
  * **void bl999_wait_rx()** - blocks execution until message from sensor(s) will be received
  * **boolean bl999_wait_rx_max(unsigned long milliseconds)** - blocks execution until message from sensor(s) will be received
    but not more than for passed amount of milliseconds. Returns `true` if message has been receieved
    `false` otherwise
  * **boolean bl999_have_message()** - 
    returns `true` if message from any sensor was recieved, `false` otherwise.
    **NOTE:** message will not be overridden with furrther messages
    until it will be read by the client using **bl999_get_message(BL999Info& info)** method.
    **NOTE2:** this function does not take in account check sum, so it will return `true` even if message check sum is incorrect  
  * **boolean bl999_get_message(BL999Info& info)** - 
    if message was recieved (matches check sum or not)
    it will be written to the passed `info` structure.
    Returns `true` if message received and check sum matches
    returns `false` otherwise
