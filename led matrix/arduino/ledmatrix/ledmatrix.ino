/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
//Pin connected to Pin 12 of 74HC595 (Latch)
int latchPin = 9;
//Pin connected to Pin 11 of 74HC595 (Clock)
int clockPin = 10;
//Pin connected to Pin 14 of 74HC595 (Data)
int dataPin = 11;


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
//  pinMode(led1, OUTPUT); 
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

// the loop routine runs over and over again forever:
void loop() {
  //count from 0 to 255
  for (int i = 0; i < 254; i++) {
    //set latchPin low to allow data flow
    digitalWrite(latchPin, LOW);
    //for (int j = 0; j < 9; j++) {
    shiftOut(0); 
    //}
    shiftOut(255); 
    //set latchPin to high to lock and send data
    digitalWrite(latchPin, HIGH);
    //delay(1);
  }

  
  

  
//just so we know it is still alive
  digitalWrite(led0, HIGH);   // turn the LED on (HIGH is the voltage level)
//  digitalWrite(led1, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led0, LOW);    // turn the LED off by making the voltage LOW
//  digitalWrite(led1, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);               // wait for a second

}
