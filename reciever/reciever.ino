#include <Arduino.h>

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


volatile unsigned int pwm_high_length = 0;
volatile unsigned int pwm_low_length = 0;
volatile unsigned int prev_time_rising = 0;
volatile unsigned int prev_time_falling = 0;
volatile byte state = 0;

// Example of the data array:
//
// tetrad:        T1    ||     T2    ||     T3    ||     T4    ||     T5
// bit:      A0|A1|A2|A3||B0|B1|B2|B3||C0|C1|C2|C3||D0|D1|D2|D3||E0|E1|E2|E3
//
// tetrad:        T6    ||     T7    ||     T8    ||     T9
// bit:      F0|F1|F2|F3||G0|G1|G2|G3||H0|H1|H2|H3||I0|I1|I2|I3
//
// A0-A3,B2-B3  - UUID which is randomly set once sensor is turned on
//                and it not changed until next power off
//
// B0-B1  - Number of the sensor: 01 - 1, 10 - 2, 11 - 3
//
// C0 - battery info: 0 - battery is ok, 1 - battery is low
//
// C1-C3 - unknown
//
// D0-F3 - temperature written backwards and multiplied by 10. For example:
//         1010|0011|0000 = 19.7° (Celsius) because 0000|1100|0101 = 197
//         negative temperature is stored in two's complement code, for example:
//         0010|1001|1111 = -10.8° (Celsius) because
//         01101100 (inverted all bits and added 1) = 108
//
// G0-H3 - humidity written backwards as 100 - HUMIDITY in two's compliment code, for example:
//          1110|1011 = 59% because 101001 (inverted all bits and add 1) = 41, and 100 - 41 = 59
//
// I0-I3 - 4 check sum which is calculated as a sum of less significant bits
//            of check sum of the tetrads T1-T8 written backwards.
//            for example let's look at the example message:
//            1011|1100|0100|0000|1111|0000|0010|1111|0010
//            sum of the tetrad T1-T8 where each bit written backwards = 110100,
//            lets take last 4 bits and wrote them backward:
//            0010 which is equal to T9 tetrad

volatile byte data[DATA_ARRAY_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
    Serial.begin(115200);
    // when pin D2 goes high, call the rising function
    attachInterrupt(0, rising, RISING);
}

void loop() { }

void rising() {
    attachInterrupt(0, falling, FALLING);
    prev_time_rising = micros();

    pwm_low_length = micros() - prev_time_falling;

    if (state % 2 == 0) {
        //Clear state to 0 since we are now in ivalid state
        setState(0, true);
    } else {
        if (state == 1) {
            //We expect start bit to match current LOW pulse
            setState(2, matchStartBit(pwm_low_length));
            return;
        }

        byte newState = state + 1;
        boolean matchOne = matchOneBit(pwm_low_length);
        boolean matchZero = matchZeroBit(pwm_low_length);

        //We expect value bit (0 or 1) to match current LOW pulse
        setState(newState, matchOne || matchZero);

        if (state == newState) {
            //Ok, we've read next value bit, lets calc it's number
            int bitNumber = (state - 1) / 2 - 1;

            //fill data array with this next value bit
            fillDataArray(bitNumber, matchOne);

            if (bitNumber == DATA_BITS_AMOUNT - 1) {

                //This was the last value bit
                //All is done we can return the whole message to the client!
                printDataArray();

                //clear state to zero since nothing more we can do
                setState(0, true);
            }
        }
    }
}

void falling() {
    attachInterrupt(0, rising, RISING);

    //remember current time
    //it will be used in raising function
    prev_time_falling = micros();

    //calc length of last HIGH pulse
    pwm_high_length = micros() - prev_time_rising;

    if (state % 2 == 0) {
        //We only can proceed is thate is even: 0, 2, 3 - after this state
        //we always expect divider HIGH pulse to happen
        setState(state + 1, matchDivider(pwm_high_length));
    } else {
        //Clear state to zero since it happens that we are in invalid state
        //at this point
        setState(0, true);
    }
}

void printDataArray() {
    Serial.println("==== Sensor data captured ===");

    for (byte i = 0; i < DATA_ARRAY_SIZE; i++) {
        printByteAsBitSet(data[i]);
        Serial.print("|");
    }
    Serial.println();

    byte sensorNumber = data[1] >> 2;
    byte powerOnUUID = (data[0] << 2) | (data[1] & 3);
    byte powerState = data[2] >> 3;
    float temperature = (float)getTemperature() / 10;
    byte humidity = getHumidity();
    boolean checkSumMatches = checkSumMatch();

    Serial.print("No: ");
    Serial.println(sensorNumber);

    Serial.print("Power UUID: ");
    Serial.println(powerOnUUID);

    Serial.print("Power state: ");
    Serial.println(powerState == 0 ? "normal" : "low");

    Serial.print("Temperature: ");
    Serial.println(temperature);

    Serial.print("Humidity: ");
    Serial.println(humidity);

    Serial.print("CheckSumMatch: ");
    printByteAsBitSet(checkSumMatch());
    Serial.println();
}

void printByteAsBitSet(byte b) {
    Serial.print((b & 8) >> 3);
    Serial.print((b & 4) >> 2);
    Serial.print((b & 2) >> 1);
    Serial.print(b & 1);
}

byte checkSumMatch() {

    //Sum first 8 tetrads
    int sum = 0;
    for (byte i = 0; i < DATA_ARRAY_SIZE - 1; i++) {
        sum += getLastFourBitsReversed(data[i]);
    }

    //returns true if calculated check sum matches received
    //return getLastFourBitsReversed(sum) == data[DATA_ARRAY_SIZE - 1];
    return getLastFourBitsReversed(sum);
}

int getTemperature() {
    int temperature = 0;

    temperature = (((int)getLastFourBitsReversed(data[5]) << 8)
                   | ((int)getLastFourBitsReversed(data[4]) << 4)
                   | (int)getLastFourBitsReversed(data[3]));

    if ((data[5] & 1) == 1) {
        //negative number, use two's compliment conversion
        temperature = ~temperature + 1;

        //clear higher bits
        temperature = temperature & 4095;
    }

    return temperature;
}

int getHumidity() {

    //negative number, use two's compliment conversion
    int humidity = ((int)(getLastFourBitsReversed(data[7]) << 4)
            | (int)getLastFourBitsReversed(data[6]));

    humidity = ~humidity + 1;

    return 100 - humidity;
}

void fillDataArray(byte bitNumber, boolean isOne) {
    byte dataArrayIndex = bitNumber / BITS_PER_PACKET;
    boolean isFirstBitInTetrad = bitNumber % 4 == 0;

    if (isFirstBitInTetrad) {
        //clear tetrad since it could be filled with random data
        data[dataArrayIndex] = 0;
    }

    data[dataArrayIndex] = (data[dataArrayIndex] << 1) | isOne;
}

//reverse last 4 bits and return
//so 0101 -> 1010, 1100 -> 0011
byte getLastFourBitsReversed(int b) {
    byte inverted = 0;

    inverted = inverted | ((b & 1) << 3); //(byte & 0001) << 3 so could be 1000
    inverted = inverted | (b & 2) << 1; //(byte & 0010) << 1 so could be 0100
    inverted = inverted | (b & 4) >> 1; // (byte & 0100) >> 1 so could be 0010
    inverted = inverted | (b & 8) >> 3; // (byte & 1000) >> 3 so could be 0001

    return inverted;
}

// Matcher for divider bit
boolean matchDivider(int value) {
    return match(value, DIVIDER_PULSE_LENGTH, DIVIDER_THRESHOLD);
}

// Matcher for start bit
boolean matchStartBit(int value) {
    return match(value, START_BIT_LENGTH, START_BIT_THRESHOLD);
}

// Matcher for binary 1
boolean matchOneBit(int value) {
    return match(value, BIT_1_LENGTH, REGULAR_BIT_THRESHOLD);
}

// Matcher for binary 0
boolean matchZeroBit(int value) {
    return match(value, BIT_0_LENGTH, REGULAR_BIT_THRESHOLD);
}

//Whether pulse length value matches specified constant with specified threshold
boolean match(int value, int mathConst, int threshold) {
    return value > mathConst - threshold && value < mathConst + threshold;
}

//set state to new value but only when condition is true
void setState(byte st, boolean condition) {
    state = condition ? st : 0;
}
