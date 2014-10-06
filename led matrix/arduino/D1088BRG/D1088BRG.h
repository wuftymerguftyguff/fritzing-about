/*
  D1088BRG Created by Jeff Arthur 4/10/2014
  Released into the public domain.
*/
#ifndef D1088BRG_h
#define D1088BRG_h

#include "Arduino.h"
#include <inttypes.h>
//TimerOne for Timer interrupt
//#include <TimerOne.h>



#define DISPLAYLENGTH 8

class TimerOne;

class D1088BRG
{
  public:
    D1088BRG(uint8_t latchPin,uint8_t clockPin,uint8_t dataPin);
    unsigned char message[8];
    void _screenUpdate();
    static void _screenUpdate_wrapper();
    void _selftest();
    uint8_t getLED();
    void update();
  private:
    uint8_t _latchPin;
    uint8_t _clockPin;
    uint8_t _dataPin;
    uint8_t _led[8];
    bool _ledState;
    unsigned long _previousMillis;
    unsigned char _display[DISPLAYLENGTH][8];
    int _displayColumns;
    
    void _writeToDisplay();
    void _clearDisplay();
 
    
    void _shiftOut(byte dataOut);
    void _updateDisplay(int column);
    
   
    
};

#endif

