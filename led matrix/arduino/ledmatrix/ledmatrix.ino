/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
#include "font.h" 
#include <math.h>
#include <TimerOne.h>
#define DISPLAYLENGTH 7

//Pin connected to Pin 12 of 74HC595 (Latch)
int latchPin = 9;
//Pin connected to Pin 11 of 74HC595 (Clock)
int clockPin = 10;
//Pin connected to Pin 14 of 74HC595 (Data)
int dataPin = 11;

uint8_t led[8];

long previousMillis = 0;

// create array to store the whole of the display in memory
unsigned char display[DISPLAYLENGTH][8];

// the message to display for now
unsigned char message[] = " HELLO ";

// the number of columns in the display
int displayColumns = DISPLAYLENGTH * 8;

// the first column of the display that we are displaying
volatile int currentColumn = 0;

// give it a name:
int led0 = 13;
volatile boolean ledState = LOW;
//int led1 = 9;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  //Serial.begin(115200);
  pinMode(led0, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT); 
  //display start state (all on)
  led[0] = B11111111;
  led[1] = B11111111;
  led[2] = B11111111;
  led[3] = B11111111;
  led[4] = B11111111;
  led[5] = B11111111;
  led[6] = B11111111;
  led[7] = B11111111;
 
 // fill the whole of the display with the default content 
  cleardisplay();
  
  Timer1.initialize(2900);
  Timer1.pwm(11, 1024);  
  Timer1.attachInterrupt(screenUpdate);
  
  //self test the display
  selftest();
  writeToDisplay();
}


void writeToDisplay() {
  for (int i=0;i<DISPLAYLENGTH;i++) {
            int letter = (int) message[i];
            memcpy(&display[i],&font_8x8[letter-32],sizeof(led));
  }
}



void cleardisplay() {
  memset(&display[0], 0x00, sizeof(display));
}

void selftest() {
  for (int i=0;i<5;i++){
    for (int k = 0; k < 9; k++) { 
      led[k] = ~led[k];
    }
    delay(200);
  }
  for (int k = 0; k < 8; k++) { 
    for (int j = 0; j < 9; j++) {
     led[k] =  pow(2,j);
     delay(20);
    }
  }
}

void screenUpdate() {
  uint8_t row = B10000000;
   for(byte k = 0; k < 9; k++) {                                                                     
    //Serial.println(k);
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

void updateDisplay(int column) {
  unsigned char *char1;
  unsigned char *charbuffer;
  int firstchar;
  int columnBitOffset = column % 8;
  if ( columnBitOffset != 0) {  // if we are not exactly aligned with char boundary in the display start processing at out bit offset inside that char
    firstchar = column / 8;  //which char does the current column start in?
    char1 = display[firstchar]; // grab that char from the display
    char1 += columnBitOffset; // now move over how many bytes we are offsetby
    charbuffer = char1;  // and write it to the charbuffer
 
  } else { // if we aligned with complete char then just grab that char to the charbuffer
    charbuffer =  display[column / 8];
  }
  memcpy(&led[0],charbuffer,sizeof(led));  //copy the charbugger to the pysical display memory map.
}

int achar=48;
// the loop routine runs over and over again forever:
void loop() {
  
          
          unsigned long currentMillis = millis();          
          if( currentMillis - previousMillis > 50 ) {
            updateDisplay(currentColumn);
            currentColumn++;
            if ( currentColumn > displayColumns - 8 ) currentColumn=0;
            ledState = !ledState;
            digitalWrite(led0, ledState);
            previousMillis = currentMillis;
           }
        

}
