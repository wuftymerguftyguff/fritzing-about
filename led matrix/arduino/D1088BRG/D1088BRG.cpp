/*
  D1088BRG.cpp - Library for handling XSM D1088BRG
  Created by Jeff Arthur 04/10/2014.
  Released into the public domain.
*/

//#include "Arduino.h"
// math lib for pow
//#include <math.h>
// the fonts
#include <D1088BRG_font.h>
//#include <TimerOne.h>
#include "D1088BRG.h"

static D1088BRG *interruptObject = NULL; 


D1088BRG::D1088BRG(uint8_t latchPin,uint8_t clockPin,uint8_t dataPin)
{
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(10, OUTPUT);

  _latchPin = latchPin;
  _clockPin = clockPin;
  _dataPin = dataPin;
  _previousMillis = 0;
  _currentColumn = 0;
  volatile bool doScreenUpdate = false;
  unsigned char _led[8];
  unsigned char _display[DISPLAYLENGTH][8];
  _displayColumns = DISPLAYLENGTH * 8;
  _ledState = false;
  //unsigned char *message = "H";
  //unsigned char pants[] = "Pants";
}

void D1088BRG::initialize() {
  interruptObject = this;  //for interrupt handler
    
    // Set up Timer1 for interrupt:
  /*  
  cli();  
  TIMSK1  = 0;
  TCCR1A  = _BV(WGM11); // Mode 14 (fast PWM), OC1A off
  TCCR1B  = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // Mode 14, no prescale
  ICR1    = 1000000;
  TIMSK1 |= _BV(TOIE1); // Enable Timer1 interrupt
  sei();                // Enable global interrupts
  /*
  // End Timer1 Setup
  _clearDisplay();
  _selftest();
  _clearDisplay();
  //writeToDisplay();
   */
   
    // initialize Timer1
    cli();             // disable global interrupts
    TCCR1A = 0;        // set entire TCCR1A register to 0
    TCCR1B = 0;
    
    // enable Timer1 overflow interrupt:
    TIMSK1 = (1 << TOIE1);
    // Set CS10 bit so timer runs at clock speed:
    //TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS11);
    //TCCR1B |= (1 << CS12);
    // enable global interrupts:
    sei();
 
}

void D1088BRG::_screenUpdate_wrapper()
{
    if (interruptObject) {
        interruptObject->_screenUpdate();
    }
}

ISR(TIMER1_OVF_vect) { // ISR_BLOCK important -- see notes later
  D1088BRG::_screenUpdate_wrapper();  // Call refresh func for active display
  TIFR1 |= TOV1;                  // Clear Timer1 interrupt flag
} 

void D1088BRG::writeToDisplay(char *message,int msgSz) {
        //_clearDisplay();
    //unsigned char message[DISPLAYLENGTH]= " Pants! ";
	    TIMSK1 = 0;
        for (int i=0;i<msgSz-1;i++) {
        int letter = (int) message[i];
        //Serial.println(letter);
        memcpy(&_display[i],&font_8x8[letter-32],sizeof(_led));
	    }
	    TIMSK1 = (1 << TOIE1);
}


void D1088BRG::_clearDisplay() {
    memset(&_display, 0x00, sizeof(_display));
	memset(&_led[0], 0xFF, sizeof(_led));
	//memcpy(&_led[0],&font_8x8[33-32],sizeof(_led));
}

void D1088BRG::_selftest() {
     for (int i=0;i<5;i++){
    for (int k = 0; k < 9; k++) { 
      _led[k] = ~_led[k];
    }
    delay(200);
  }
  for (int k = 0; k < 8; k++) { 
    for (int j = 0; j < 9; j++) {
     _led[k] =  pow(2,j);
     delay(20);
    }
  }
}

void D1088BRG::_screenUpdate() {
    uint8_t row = B10000000;
    for(byte k = 0; k < 9; k++) {
        // Open up the latch ready to receive data
        digitalWrite(_latchPin, LOW);
        _shiftOut(~row );
        _shiftOut(_led[k]); // LED array
        // Close the latch, sending the data in the registers out to the matrix
        digitalWrite(_latchPin, HIGH);
        row = row >> 1;
    }
}

void D1088BRG::_shiftOut(byte dataOut) {
    // Shift out 8 bits LSB first,
    // on rising edge of clock
    boolean pinState;
    //clear shift register ready for sending data
    digitalWrite(_dataPin, LOW);
    digitalWrite(_clockPin, LOW);
    // for each bit in dataOut send out a bit
    for (int i=0; i<8; i++) {
        //set clockPin to LOW prior to sending bit
        digitalWrite(_clockPin, LOW);
        // if the value of DataOut and (logical AND) a bitmask
        // are true, set pinState to 1 (HIGH)
        if ( dataOut & (1<<i) ) {
            pinState = HIGH;
        }
        else {
            pinState = LOW;
        }
        //sets dataPin to HIGH or LOW depending on pinState
        digitalWrite(_dataPin, pinState);
        //send bit out on rising edge of clock
        digitalWrite(_clockPin, HIGH);
    }
    //stop shifting out data
    digitalWrite(_clockPin, LOW);
}

uint8_t D1088BRG::getLED() {
    return _led[0];
}

void D1088BRG::_updateDisplay(int column) {
	TIMSK1 = 0;
    unsigned char *char1;
    unsigned char *charbuffer;
    int firstchar;
    int columnBitOffset = column % 8;
    if ( columnBitOffset != 0) {  // if we are not exactly aligned with char boundary in the display start processing at out bit offset inside that char
        firstchar = column / 8;  //which char does the current column start in?
        char1 = _display[firstchar]; // grab that char from the display
        char1 += columnBitOffset; // now move over how many bytes we are offset by
        charbuffer = char1;  // and write it to the charbuffer
        
    } else { // if we aligned with complete char then just grab that char to the charbuffer
        charbuffer =  _display[column / 8];
    }
    memcpy(&_led[0],charbuffer,sizeof(_led));  //copy the charbuffer to the physical display memory map.
	TIMSK1 = (1 << TOIE1);
}

void D1088BRG::update() {
    unsigned long currentMillis = millis();
    if( currentMillis - _previousMillis > 100 ) {
        _updateDisplay(_currentColumn);
        _currentColumn++;
        if ( _currentColumn > _displayColumns - 8 ) _currentColumn=0;
  //      _ledState = !_ledState;
  //      digitalWrite(13, _ledState);
        _previousMillis = currentMillis;
   }

}



