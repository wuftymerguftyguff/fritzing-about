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

//#define NPLYEAR 0

//# define if we are DEBUGGING
//#define DEBUG 

//structure
struct timeElement {
  int buffer;
  int offset;
  int numBytes;  
};

#define NPLMINUTEMARKERBITPATTERN 126

#define NPLYEAR (struct timeElement){SAVED_A_BUFFER,17,8}
#define NPLMONTH (struct timeElement){SAVED_A_BUFFER,25,5}
#define NPLDAY (struct timeElement){SAVED_A_BUFFER,30,6}
#define NPLWEEKDAY (struct timeElement){SAVED_A_BUFFER,36,3}
#define NPLHOUR (struct timeElement){SAVED_A_BUFFER,39,6}
#define NPLMINUTE (struct timeElement){SAVED_A_BUFFER,45,7}

#define NPLMINUTEMARKER (struct timeElement){A_BUFFER,52,8}




//starting offset inside per second "byte"
volatile int startingOffset = 0;

// starting state of the LED
volatile int ledState = LOW;

// time of this pulse state change
long thisPulseChange = 0;
long lastPulseChange = 0;

// an array to store the bits in the current second
// in msf each second is cplit into 10 100ms "bits"

bool second[10];

byte aBuffer[8];// buffer for 'A' bits
byte bBuffer[8];// buffer for 'B' bits

byte savedaBuffer[8]; // saved copies of the above buffers
byte savedbBuffer[8];

// the pulse (second) offset in this minute
int secondOffset = 0;


// a boolean to show if we are at the top of the minute
bool TOM = false;
bool TOS = false;

// how long we aim to pause the main loop (ms)
long plannedDelay = 25;

// hw long it takes to go around the main loop (we assume perfect timing as a starting point
long mainElapsed = 0;

// how long we need to delay to get the planned delay in reality
long actualDelay = plannedDelay;

long pulseStart,pulseEnd,pulseTimeLow,pulseTimeHigh,seconds = 0;

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
  Serial.print("RISING ");
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

void printTime() {
  char buffer [20];
  sprintf(buffer, "%02d/%02d/20%02d %02d:%02d:%02d", getTimeVal(NPLDAY),
                                      getTimeVal(NPLMONTH),
                                      getTimeVal(NPLYEAR),
                                      getTimeVal(NPLHOUR),
                                      getTimeVal(NPLMINUTE),
                                      secondOffset                            
  );
  Serial.println(buffer);
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

byte GetChunk(int start, int numBits, int buffer)  {
// return the byte 'chunk' containing the number of bits read at the starting
// bit position in the raw buffer upto 8 bits. returns a byte

  byte chunk = 0;
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
// this looks reversed as the module reversed the serial output of the carrier state
attachInterrupt(0, risingPulse, FALLING) ;
attachInterrupt(1, fallingPulse, RISING) ;

Serial.begin(115200);           // set up Serial library at 19200 bps
Serial.println("Clock Starting");  // Say something to show we have restarted.
pinMode(ledPin, OUTPUT); // set up the ledpin
// set all the values of the current second to 1

// as msf signal is on by default and is turned off to "send data"
memset(&second[0], 0x00, sizeof(second));
clearBuffers();
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
#endif
    
    //printBufferBits();
    
        
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
  
 
 
}

