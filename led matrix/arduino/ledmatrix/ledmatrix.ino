/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
#include "font.h" 
#include <TimerOne.h>
 
//Pin connected to Pin 12 of 74HC595 (Latch)
int latchPin = 9;
//Pin connected to Pin 11 of 74HC595 (Clock)
int clockPin = 10;
//Pin connected to Pin 14 of 74HC595 (Data)
int dataPin = 11;

uint8_t led[8];

long previousMillis = 0;



// give it a name:
int led0 = 13;
//int led1 = 9;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led0, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT); 
  led[0] = B11111111;
  led[1] = B11111111;
  led[2] = B11111111;
  led[3] = B11111111;
  led[4] = B11111111;
  led[5] = B11111111;
  led[6] = B11111111;
  led[7] = B11111111; 
  Timer1.initialize(5000);
  Timer1.attachInterrupt(screenUpdate);
  selftest();
}

void selftest() {
  for (int i=0;i<5;i++){
    for (int k = 0; k < 9; k++) { 
      led[k] = ~led[k];
    }
    delay(1000);
  }
}

void screenUpdate() {
  uint8_t row = B10000000;
   for(byte k = 0; k < 9; k++) {                                                                     
    Serial.println(k);
    // Open up the latch ready to receive data
    digitalWrite(latchPin, LOW); 
    shiftOut(~row ); 
    shiftOut(led[k]); // LED array
  // Close the latch, sending the data in the registers out to the matrix
    digitalWrite(latchPin, HIGH);
    row = row >> 1;
    }
  }


void shiftOut(byte dataOut) {
  // Shift out 8 bits LSB first, 
  // on rising edge of clock
  boolean pinState;
  //clear shift register ready for sending data
  digitalWrite(dataPin, LOW);
  digitalWrite(clockPin, LOW);
  // for each bit in dataOut send out a bit
  for (int i=0; i<=7; i++) {
  //set clockPin to LOW prior to sending bit
  digitalWrite(clockPin, LOW);
  // if the value of DataOut and (logical AND) a bitmask
  // are true, set pinState to 1 (HIGH)
  if ( dataOut & (1<<i) ) {
    pinState = HIGH;
  }
  else {
    pinState = LOW;
  }
  //sets dataPin to HIGH or LOW depending on pinState
  digitalWrite(dataPin, pinState);
  //send bit out on rising edge of clock 
  digitalWrite(clockPin, HIGH);
  }
  //stop shifting out data
  digitalWrite(clockPin, LOW);
}

int achar=48;
// the loop routine runs over and over again forever:
void loop() {
          //Serial.println("Looping");          
          //Serial.println(currentMillis - previousMillis);
          unsigned long currentMillis = millis();
          if( currentMillis - previousMillis > 500 ) { 
             memcpy(&led[0],&font_8x8[achar-32],sizeof(led));
             previousMillis = currentMillis;
             achar++;
           if ( achar > 122 ) achar=48;
          }

/*
  
//just so we know it is still alive
  digitalWrite(led0, HIGH);   // turn the LED on (HIGH is the voltage level)
//  digitalWrite(led1, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led0, LOW);    // turn the LED off by making the voltage LOW
//  digitalWrite(led1, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);               // wait for a second
  
  */

}
