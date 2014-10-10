
#include <MemoryFree.h>
#include <D1088BRG.h>
#include <stdio.h>
#define wwvbPin 2
#define ledPin 13
#define LOW 0
#define HIGH 1

#define A_BUFFER 0
#define B_BUFFER 1
#define SAVED_A_BUFFER 2
#define SAVED_B_BUFFER 3

#define CURRENTCENTURY 20

//#define DEBUG 1


//Pin connected to Pin 12 of 74HC595 (Latch)
uint8_t latchPin = 9;
//Pin connected to Pin 11 of 74HC595 (Clock)
uint8_t clockPin = 10;
//Pin connected to Pin 14 of 74HC595 (Data)
uint8_t dataPin = 11;

D1088BRG D1088BRG(latchPin,clockPin,dataPin);

//#define NPLYEAR 0

//# define if we are DEBUGGING
//#define DEBUG 

//structure
struct timeElement {
  uint8_t buffer;
  uint8_t offset;
  uint8_t numBytes;  
};

#define NPLMINUTEMARKERBITPATTERN 126

#define NPLYEAR (struct timeElement){SAVED_A_BUFFER,17,8}
#define NPLMONTH (struct timeElement){SAVED_A_BUFFER,25,5}
#define NPLDAY (struct timeElement){SAVED_A_BUFFER,30,6}
#define NPLWEEKDAY (struct timeElement){SAVED_A_BUFFER,36,3}
#define NPLHOUR (struct timeElement){SAVED_A_BUFFER,39,6}
#define NPLMINUTE (struct timeElement){SAVED_A_BUFFER,45,7}

#define NPLMINUTEMARKER (struct timeElement){A_BUFFER,52,8}

#define NPLYEARCHECKDIGIT (struct timeElement){A_BUFFER,54,1}
#define NPLDAYCHECKDIGIT (struct timeElement){A_BUFFER,55,1}
#define NPLDOWCHECKDIGIT (struct timeElement){A_BUFFER,56,1}
#define NPLTIMECHECKDIGIT (struct timeElement){A_BUFFER,57,1}

#define NPLYEARBITS (struct timeElement){A_BUFFER,17,8}
#define NPLDAYBITS (struct timeElement){A_BUFFER,25,11}
#define NPLDOWBITS (struct timeElement){A_BUFFER,36,3}
#define NPLTIMEBITS (struct timeElement){A_BUFFER,39,13}



//starting offset inside per second "byte"
volatile uint8_t startingOffset = 0;

// starting state of the LED
volatile uint8_t ledState = LOW;

// time of this pulse state change
unsigned long thisPulseChange = 0;
unsigned long lastPulseChange = 0;

// an array to store the bits in the current second
// in msf each second is cplit into 10 100ms "bits"

bool second[10];

byte aBuffer[8];// buffer for 'A' bits
byte bBuffer[8];// buffer for 'B' bits

byte savedaBuffer[8]; // saved copies of the above buffers
byte savedbBuffer[8];

// the pulse (second) offset in this minute
uint8_t secondOffset = 0;


// a boolean to show if we are at the top of the minute
bool TOM = false;
bool TOS = false;

// hw long it takes to go around the main loop (we assume perfect timing as a starting point
long mainElapsed = 0;

long pulseStart,pulseEnd,pulseTimeLow,pulseTimeHigh = 0;

long pulseWidth;
long lastPulseWidth;

boolean pulseStatus = false;
boolean childPulse = false;
long lastPulse = 0;
int sigWas = LOW;
int carrierState;

int secondMillis;

int pulseTime = 0;

long startTimeHigh = 0;
long startTimeLow = 0;

bool parity(uint16_t v) {
  bool parity = false;  // parity will be the parity of v
  while (v)
  {
    parity = !parity;
    v = v & (v - 1);
  }
}


bool isParityValid(struct timeElement element,struct timeElement checkdigit) {
  uint16_t bits = GetChunk(element.offset,element.numBytes,element.buffer);
  bool calculatedParity=parity(bits);
  byte checkdigitbit = GetChunk(checkdigit.offset,checkdigit.numBytes,checkdigit.buffer); 
  Serial.print(F("Bits : "));
  Serial.println(bits);
  Serial.print(F("Calculated Parity : "));
  Serial.println(calculatedParity);
  Serial.print(F("Parity : "));
  Serial.println(checkdigitbit);
}

void setBits(int width,int level) {
  
  int numBitsToSet = max(width/100,1);  // Each "bit" is 100ms, but we have to set at least one (one bit pulses are oten reported as < 100 ms !!
  int bitOffset = startingOffset;       // Which bit should we set next?
  startingOffset += numBitsToSet;       // what bit should we start with the next time we are called?
  
  if ( level == LOW ) return;  //return immediately for low bits as these are defaulted to 1 anyway
  
  
  for (int i=0;i < numBitsToSet;i++) {
    //Serial.print(" setting bit ");
    //Serial.print(bitOffset);
    //Serial.print(" of ");
    //Serial.print(numBitsToSet);
    //Serial.print(" bits "); 
    second[bitOffset] = true;
    bitOffset++; 
  } 
  
 
  
}

void pulseChange() {
  // save the timestamp of the last pulse interrupt
  lastPulseChange = thisPulseChange;
  
  // grab the start time of this pulse interrupt
  thisPulseChange = millis();
  
  // save the duration of the last pulse
  lastPulseWidth = pulseWidth;
  
  //calculate the duration of this pulse
  pulseWidth = thisPulseChange - lastPulseChange; 
  
  //toggle the state of the led
  ledState = !ledState;
  
  // set the LED with the ledState of the variable:
  digitalWrite(ledPin, ledState);
}

void risingPulse() {
  
  // do the generic stuff that applies for any pulse
  pulseChange();
  
#ifdef DEBUG  
  Serial.print(F("RISING "));
  Serial.print("HIGH: ");
  Serial.print(pulseWidth);
  Serial.print(" ");
#endif  

  setBits(pulseWidth,HIGH);
}

void fallingPulse() {
  
  // do the generic stuff that applies for any pulse
  pulseChange(); 
  
#ifdef DEBUG  
  Serial.print("FALLING ");
  Serial.print("LOW: ");
  Serial.print(pulseWidth);
  Serial.print(" ");
#endif  
  
  setBits(pulseWidth,LOW);
  
  
  if ( pulseWidth >= 450 ) {
    TOS = true;
    //secondOffset++;
    //Serial.println(GetChunk(secondOffset - 8,8,A_BUFFER));
    //Serial.println(NPLMINUTEMARKERBITPATTERN);
      
    fillBuffers(second[1],second[2]);
    memset(&second[0], 0x00, sizeof(second));
    secondOffset++;
    startingOffset = 0;
  }
/*  
  if ( getTimeByte(NPLMINUTEMARKER) == NPLMINUTEMARKERBITPATTERN ) {
      Serial.println("1st they match");
      Serial.print("\nTOM *************\n");
      secondOffset = 0;
   }
*/
    

    if ( GetChunk(secondOffset - 8,8,A_BUFFER) == NPLMINUTEMARKERBITPATTERN ) {
      
/*      
#define NPLYEARCHECKDIGIT (struct timeElement){A_BUFFER,54,1}
#define NPLDAYCHECKDIGIT (struct timeElement){A_BUFFER,55,1}
#define NPLDOWCHECKDIGIT (struct timeElement){A_BUFFER,56,1}
#define NPLTIMECHECKDIGIT (struct timeElement){A_BUFFER,57,1}

#define NPLYEARBITS (struct timeElement){A_BUFFER,17,8}
#define NPLDAYBITS (struct timeElement){A_BUFFER,25,11}
#define NPLDOWBITS (struct timeElement){A_BUFFER,36,3}
#define NPLTIMEBITS (struct timeElement){A_BUFFER,39,13}
*/
      isParityValid(NPLYEARBITS,NPLYEARCHECKDIGIT);
      saveBuffers(); 
      clearBuffers();
      secondOffset = 0;
  } 


}

void printBufferBits() {

  for (int i=0;i<=secondOffset;i++) {
    int bufferElement=i / 8;
    int bufferElementOffset = i % 8 ^ 0x07 ;
    Serial.print(bitRead(aBuffer[bufferElement],bufferElementOffset));  
    //Serial.print(bitRead(bBuffer[bufferElement],bufferElementOffset)); 
    }
    
  Serial.print("\n");
  // bcd messing
 // Serial.print("                 ");
  /*
  for (int i=17;i<=24;i++) {
    int bufferElement=i / 8;
    int bufferElementOffset = i % 8 ^ 0x07 ;
    Serial.print(bitRead(aBuffer[bufferElement],bufferElementOffset));   
  }  
  
  Serial.print("I               II Year I");
  */
  
}

uint8_t nmeaChecksum(char arr[]) {
  uint8_t sum = 0x00;
  if (arr[0] != 36) return sum;
  int cnt = 1;
  while (true) {
    if (arr[cnt] == 42) return sum;
    sum = sum^arr[cnt];
    cnt++;
  }
}

void printTime() {
  Serial.print("freeMemory()=");
  Serial.println(freeMemory());
  char buffer [DISPLAYLENGTH];
  int nplHour = getTimeVal(NPLHOUR) - 1;
  int nplMinute = getTimeVal(NPLMINUTE);
  int nplDay = getTimeVal(NPLDAY);
  int nplMonth = getTimeVal(NPLMONTH);
  int nplYear = getTimeVal(NPLYEAR);
  // $--ZDA,hhmmss.ss,xx,xx,xxxx,xx,xx*hh<CR><LF>
  /*
  sprintf(buffer, " %02d/%02d/20%02d %02d:%02d:%02d ", getTimeVal(NPLDAY),
                                      getTimeVal(NPLMONTH),
                                      getTimeVal(NPLYEAR),
                                      getTimeVal(NPLHOUR),
                                      getTimeVal(NPLMINUTE),
                                      secondOffset                            
  );
 */

   // $--ZDA,hhmmss.ss,xx,xx,xxxx,xx,xx*hh<CR><LF>
  sprintf(buffer, "$GPZDA,%02d%02d%02d.00,%02d,%02d,20%02d,+1,00*",
                       nplHour,
                       nplMinute,
                       secondOffset,
                       nplDay,
                       nplMonth,
                       nplYear);

  sprintf(buffer,"%s%02x",buffer,nmeaChecksum(buffer));
  Serial.println(buffer);
  
  //"$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*");
  //Serial.println("$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*");
   sprintf(buffer, "$GPRMC,%02d%02d%02d,A,4916.45,N,12311.12,W,000.5,054.7,%02d%02d%02d,020.3,E*",
                     nplHour,
                     nplMinute,
                     secondOffset,
                     nplDay,
                     nplMonth,
                     nplYear);
  sprintf(buffer,"%s%02x",buffer,nmeaChecksum(buffer));
  Serial.println(buffer);
                     
 
  //Serial.println(nmeaChecksum(buffer));
  D1088BRG.writeToDisplay(buffer,sizeof(buffer));

  
}

int getTimeVal(struct timeElement element) {
    return bcdToDec(GetChunk(element.offset,element.numBytes,element.buffer));  
}

byte getTimeByte(struct timeElement element) {
  return GetChunk(element.offset,element.numBytes,element.buffer);
}

byte decToBcd(byte val)			// Convert normal decimal numbers to binary coded decimal
{
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)			// Convert binary coded decimal to normal decimal numbers
{
  return ( (val/16*10) + (val%16) );
}

uint16_t GetChunk(int start, int numBits, int buffer)  {
// return the byte 'chunk' containing the number of bits read at the starting
// bit position in the raw buffer upto 8 bits. returns a byte

  uint16_t chunk = 0;
  byte bitVal = 0;
  int counter = numBits - 1;
  int bitCounter = 0;				// a count of the number of "1" bits

  for(int i = start;i < start + numBits;i++)					// loop for numBits
	{
                if ( buffer == A_BUFFER ) {
		  bitVal = bitRead(aBuffer[abs(i/8)], (i % 8) ^ 0x07);	// get the bit from the buffer
                } else if  ( buffer == B_BUFFER ) {
                  bitVal = bitRead(bBuffer[abs(i/8)], (i % 8) ^ 0x07);	// get the bit from the buffer
                } else if  ( buffer == SAVED_A_BUFFER ) {
                  bitVal = bitRead(savedaBuffer[abs(i/8)], (i % 8) ^ 0x07);	// get the bit from the buffer
                } else if  ( buffer == SAVED_B_BUFFER ) {
                  bitVal = bitRead(savedbBuffer[abs(i/8)], (i % 8) ^ 0x07);	// get the bit from the buffer    
                } else {
                  bitVal = 0;
                }
		bitWrite(chunk,counter,bitVal);							// write the bit to "chunk"
		if(bitVal) bitCounter++;								// if it's a "1" increment the bitCounter
		counter--;												// decrement the counter
	}

  return chunk;
}

void clearBuffers() {
  memset(&aBuffer[0], 0x00, sizeof(aBuffer));
  memset(&bBuffer[0], 0x00, sizeof(bBuffer));
}

void clearSavedBuffers() {
  memset(&savedaBuffer[0], 0x00, sizeof(savedaBuffer));
  memset(&savedbBuffer[0], 0x00, sizeof(savedbBuffer));
}

void saveBuffers() {
  memcpy(&savedaBuffer[0],&aBuffer[0],sizeof(aBuffer));
  memcpy(&savedbBuffer[0],&bBuffer[0],sizeof(bBuffer));
}

void fillBuffers(bool A,bool B) {
  int bufferElement=secondOffset / 8;
  int bufferElementOffset = secondOffset % 8 ^ 0x07 ;
  
  bitWrite(aBuffer[bufferElement],bufferElementOffset,A);
  bitWrite(bBuffer[bufferElement],bufferElementOffset,B);
  
  
  /*
  
  Serial.print(secondOffset);
  Serial.print(" ");
  
  Serial.print(bufferElement);
  Serial.print(" ");
  
  Serial.print(bufferElementOffset);
  Serial.print(" ");
  
  Serial.print(A);
  Serial.print(B);

  
  Serial.print(" ");
 
  Serial.print(bitRead(aBuffer[bufferElement],bufferElementOffset));
  Serial.println(bitRead(bBuffer[bufferElement],bufferElementOffset));
  
  */
  
}
  
void setup() {

D1088BRG.initialize();

// this looks reversed as the module reversed the serial output of the carrier state
attachInterrupt(0, risingPulse, FALLING) ;
attachInterrupt(1, fallingPulse, RISING) ;

Serial.begin(9600);           // set up Serial library at 19200 bps
Serial.println(F("Clock Starting"));  // Say something to show we have restarted.
pinMode(ledPin, OUTPUT); // set up the ledpin
// set all the values of the current second to 1

// as msf signal is on by default and is turned off to "send data"
memset(&second[0], 0x00, sizeof(second));
clearBuffers();
clearSavedBuffers();
}

void loop() {
  if ( TOS == true ) {
 
#ifdef DEBUG
    // loop thru the 10 bits in the prior second and print them ou
    Serial.print(" ");
    for (int i=0;i<=9;i++) { 
      Serial.print(second[i]); 
    }
    

    Serial.print(" ");
    Serial.print(secondOffset);
    Serial.print(":");
    //Display the A and B Bits
    for (int i=1;i<=2;i++) { 
      Serial.print(second[i]); 
    }
    Serial.print("\n");
    printBufferBits();
#endif
    
    
    
        
#ifdef DEBUG
    Serial.print("TOS "); 
#endif 
    printTime();
    TOS = false;
    
    
    
    if ( TOM == true ) {
        printTime();
       
        TOM = false;   
     }
    
   
  }
  
 
D1088BRG.update(); 
}

