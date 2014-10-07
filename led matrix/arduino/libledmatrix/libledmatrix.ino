#include <D1088BRG.h>
//#include <TimerOne.h>                                                          `  


//Pin connected to Pin 12 of 74HC595 (Latch)
int latchPin = 9;
//Pin connected to Pin 11 of 74HC595 (Clock)
int clockPin = 10;
//Pin connected to Pin 14 of 74HC595 (Data)
int dataPin = 11;

D1088BRG D1088BRG(latchPin,clockPin,dataPin);

void setup() {
  Serial.begin(9600);
  D1088BRG.initialize();
  char msg[] = " Hello Message ";
  D1088BRG.writeToDisplay(msg,sizeof(msg));
}

void loop() {
  D1088BRG.update();
  //Serial.println(D1088BRG.getLED());
  //D1088BRG._screenUpdate();
}
