#include <Arduino.h>

#define DIVIDER_PULSE_LENGTH 600
#define START_BIT_LENGTH 9000
#define BIT_1_LENGTH 3900
#define BIT_0_LENGTH 1850

#define DIVIDER_TRESHOLD 100
#define START_BIT_TRESHOLD 1000
#define REGULAR_BIT_TRESHOLD 500

#define DATA_BIT_SIZE 36
#define BITS_PER_PACKET 4
#define DATA_ARRAY_SIZE 9


volatile int pwm_high_length = 0;
volatile int pwm_low_length = 0;
volatile int prev_time_rising = 0;
volatile int prev_time_falling = 0;
volatile byte state = 0;
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
    setState(state, false);
  } else {
      if (state == 1) {
        setState(2, matchStartBit(pwm_low_length));
        return;
      }
      
      byte newState = state + 1;
      boolean matchOne = matchOneBit(pwm_low_length);
      boolean matchZero = matchZeroBit(pwm_low_length);
      
      setState(newState, matchOne || matchZero);
      if (state == newState) {
          int bitNumber = (state - 1) / 2 - 1;
          //fillDataArray(bitNumber, matchOne);
          
          if (bitNumber == DATA_BIT_SIZE) {
            //for (byte i = 0; i < DATA_ARRAY_SIZE; i++) {
            //    Serial.print(data[i]);
            //    Serial.print(" ");         
            //}
            Serial.println(matchOne);    

            setState(state, false);
          }            
      } 
  }
}
 
void falling() {
  attachInterrupt(0, rising, RISING);
  prev_time_falling = micros();
  
  pwm_high_length = micros() - prev_time_rising;

  if (state % 2 == 0) {
      setState(state + 1, matchDivider(pwm_high_length));
  } else {
      setState(state, false);
  }
}

void fillDataArray(byte bitNumber, boolean isOne) {
   byte dataArrayIndex = bitNumber / BITS_PER_PACKET;
   byte value = bitNumber ? 1 : 0;

   data[dataArrayIndex] = (data[dataArrayIndex] << 1) & value;
}

void clearDataArray(byte upTo) {
  for (byte i = 0; i <= upTo; i++) {
    data[i] = 0; 
  }
}

boolean matchDivider(int value) {
    return match(value, DIVIDER_PULSE_LENGTH, DIVIDER_TRESHOLD);
}

boolean matchStartBit(int value) {
    return match(value, START_BIT_LENGTH, START_BIT_TRESHOLD);
}

boolean matchOneBit(int value) {
    return match(value, BIT_1_LENGTH, REGULAR_BIT_TRESHOLD);
}

boolean matchZeroBit(int value) {
    return match(value, BIT_0_LENGTH, REGULAR_BIT_TRESHOLD);
}

boolean match(int value, int mathConst, int treshold) {
    return value >  mathConst - treshold && value < mathConst + treshold; 
}

void setState(byte st, boolean condition) {
    if (condition) {
        state = st;
    } else {
        state = 0;
        //if (st > 3) {
        //  clearDataArray(((st - 2) / 2 - 1) / BITS_PER_PACKET); 
        //}      
    }
}

