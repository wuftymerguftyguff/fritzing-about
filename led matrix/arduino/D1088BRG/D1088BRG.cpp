/*
  D1088BRG.cpp - Library for handling XSM D1088BRG
  Created by Jeff Arthur 04/10/2014.
  Released into the public domain.
*/

#include "Arduino.h"
// math lib for pow
#include <math.h>
// the fonts
#include <font.h>
#include <TimerOne.h>
#include "D1088BRG.h"

D1088BRG interruptObject = NULL;





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
  _led[8];
  _display[DISPLAYLENGTH][8];
  _displayColumns = DISPLAYLENGTH * 8;
  _ledState = false;
    
  _clearDisplay();
    
  Timer1.initialize(10000);
  Timer1.pwm(11, 1024);
  interruptObject = this;
  //Timer1.attachInterrupt(_screenUpdate_wrapper);
  //_selftest();
  //_writeToDisplay();
    
}

void D1088BRG::_screenUpdate_wrapper()
{
    if (interruptObject) {
        interruptObject->_screenUpdate();
    }
}

void D1088BRG::_writeToDisplay() {
    for (int i=0;i<DISPLAYLENGTH;i++) {
        int letter = (int) message[i];
        memcpy(&_display[i],&font_8x8[letter-32],sizeof(_led));
    }
}


void D1088BRG::_clearDisplay() {
    memset(&_display[0], 0xFF, sizeof(_display));
    memset(&_led[0], 0x00, sizeof(_led));
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
    for (int i=0; i<=7; i++) {
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
    unsigned char *char1;
    unsigned char *charbuffer;
    int firstchar;
    int columnBitOffset = column % 8;
    if ( columnBitOffset != 0) {  // if we are not exactly aligned with char boundary in the display start processing at out bit offset inside that char
        firstchar = column / 8;  //which char does the current column start in?
        char1 = _display[firstchar]; // grab that char from the display
        char1 += columnBitOffset; // now move over how many bytes we are offsetby
        charbuffer = char1;  // and write it to the charbuffer
        
    } else { // if we aligned with complete char then just grab that char to the charbuffer
        charbuffer =  _display[column / 8];
    }
    memcpy(&_led[0],charbuffer,sizeof(_led));  //copy the charbuffer to the physical display memory map.
}

void D1088BRG::update() {
    uint8_t currentColumn = 0;
    unsigned long currentMillis = millis();
    if( currentMillis - _previousMillis > 500 ) {
        _updateDisplay(currentColumn);
        currentColumn++;
        if ( currentColumn > _displayColumns - 8 ) currentColumn=0;
        _ledState = !_ledState;
        digitalWrite(13, _ledState);
        _previousMillis = currentMillis;
    }

}



