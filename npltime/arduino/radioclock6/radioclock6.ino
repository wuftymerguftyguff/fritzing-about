
//#include <MemoryFree.h>
//#include <D1088BRG.h>
#include <stdio.h>
#define wwvbPin 2
#define ledPin 13
#define LOW 0
#define HIGH 1

#define A_BUFFER 0
#define B_BUFFER 1
#define SAVED_A_BUFFER 2
#define SAVED_B_BUFFER 3

const byte CURRENTCENTURY=20;

//#define DEBUG 1

//Pin connected to Pin 12 of 74HC595 (Latch)
const uint8_t latchPin = 9;
//Pin connected to Pin 11 of 74HC595 (Clock)
const uint8_t clockPin = 10;
//Pin connected to Pin 14 of 74HC595 (Data)
const uint8_t dataPin = 11;

//D1088BRG D1088BRG(latchPin,clockPin,dataPin);

//#define NPLYEAR 0

//# define if we are DEBUGGING
//#define DEBUG 

//structure
struct timeElement {
  byte buffer;
  byte offset;
  byte numBytes;
  byte parityNumBytes;
  byte parityBit;  
};

const byte NPLMINUTEMARKERBITPATTERN = 126;

const timeElement NPLYEAR = {SAVED_A_BUFFER,17,8};
const timeElement NPLMONTH = {SAVED_A_BUFFER,25,5};
const timeElement NPLDAY = {SAVED_A_BUFFER,30,6};
const timeElement NPLWEEKDAY = {SAVED_A_BUFFER,36,3};
const timeElement NPLHOUR = {SAVED_A_BUFFER,39,6};
const timeElement NPLMINUTE = {SAVED_A_BUFFER,45,7};

const timeElement NPLMINUTEMARKER = {A_BUFFER,52,8};

const timeElement NPLYEARPARITY = {A_BUFFER,17,8,8,54};
const timeElement NPLDAYPARITY = {A_BUFFER,25,11,11,55};
const timeElement NPLDOWPARITY = {A_BUFFER,36,3,3,56};
const timeElement NPLTIMEPARITY = {A_BUFFER,39,13,13,57};



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

volatile byte aBuffer[8];// buffer for 'A' bits
volatile byte bBuffer[8];// buffer for 'B' bits

byte savedaBuffer[8]; // saved copies of the above buffers
byte savedbBuffer[8];

// the pulse (second) offset in this minute
volatile uint8_t secondOffset = 0;


// a boolean to show if we are at the top of the minute
volatile bool TOM = false;
volatile bool TOS = false;

// a boolean to show if we are happy we have an accurate time
// this is passed on as a GPS "lock" in the nmea data
#define GPS_LOCK "A"
#define GPS_NO_LOCK "V"
char NMEA_NAVIGATION_WARNING = *GPS_NO_LOCK;

//long pulseTimeLow,pulseTimeHigh = 0;

long pulseWidth;
long lastPulseWidth;

int pulseTime = 0;

long startTimeHigh = 0;
long startTimeLow = 0;

/*structure
struct timeElement {
  byte buffer;
  byte offset;
  byte numBytes;
  byte parityNumBytes;
  byte parityBit;  
};
*/

unsigned int oddParity(uint16_t x) {
   uint16_t y;
   y = x ^ (x >> 1);
   y = y ^ (y >> 2);
   y = y ^ (y >> 4);
   y = y ^ (y >> 8);
   y = y ^ (y >>16);
   y = ~y & 1;
   return y;   
}

boolean isParityValid(struct timeElement element) {
  //check_mem();
  uint16_t timedata = GetChunk(element.offset,element.parityNumBytes,element.buffer);
  byte sentParity = GetChunk(element.parityBit,1,B_BUFFER);
  unsigned int calcParity = oddParity(timedata);
  boolean retval = (calcParity == sentParity);
#ifdef DEBUG  
  Serial.print(timedata);
  Serial.print("\t");
  Serial.print(calcParity);
  Serial.print("\t");
  Serial.print(sentParity);
  Serial.print("\t");
  Serial.print(retval);
  Serial.print("\n");
  Serial.flush();
#endif  
  return retval;
}

/*
bool isParityValid(struct timeElement element,struct timeElement checkdigit) {
  uint16_t bits = GetChunk(element.offset,element.numBytes,element.buffer);
  bool calculatedParity=oddParity(bits);
  uint16_t checkdigitbit = GetChunk(checkdigit.offset,checkdigit.numBytes,checkdigit.buffer); 

  Serial.print(F("Bits : "));
  Serial.print(bits);
  Serial.print(F(" Calculated Parity : "));
  Serial.print(calculatedParity);
  Serial.print(F(" Parity : "));
  Serial.print(checkdigitbit);

  if ( calculatedParity == checkdigitbit ) {
    return false;
  } else {
    return true;
  }
}
*/

void setBits(int width,uint8_t level) {
  
  uint8_t numBitsToSet = max(width/100,1);  // Each "bit" is 100ms, but we have to set at least one (one bit pulses are oten reported as < 100 ms !!
  uint8_t bitOffset = startingOffset;       // Which bit should we set next?
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

    

    if ( GetChunk(secondOffset - 8,8,A_BUFFER) == NPLMINUTEMARKERBITPATTERN ) {
      TOM = true;
      /*
      if ( isParityValid(NPLYEARBITS,NPLYEARCHECKDIGIT)
            && isParityValid(NPLDAYBITS,NPLDAYCHECKDIGIT)
            && isParityValid(NPLDOWBITS,NPLDOWCHECKDIGIT) 
            && isParityValid(NPLTIMEBITS,NPLTIMECHECKDIGIT)
         ) Serial.println("bad parity");
      
         
        saveBuffers(); 
        secondOffset = 0;
        clearBuffers(); 
    */    
    } 


}

void printBufferBits() {

  for (uint8_t i=0;i<secondOffset;i++) {
    uint8_t bufferElement=i / 8;
    uint8_t bufferElementOffset = i % 8 ^ 0x07 ;
    Serial.print(bitRead(aBuffer[bufferElement],bufferElementOffset));  
    }
    
  Serial.print(F("\n"));
  for (uint8_t i=0;i<secondOffset;i++) {
    uint8_t bufferElement=i / 8;
    uint8_t bufferElementOffset = i % 8 ^ 0x07 ;
    Serial.print(bitRead(bBuffer[bufferElement],bufferElementOffset));  
    }
    
  Serial.print(F("\n"));
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
  //heapptr, * stackptr;
  //Serial.println(*heapptr);
  //Serial.println(*stackptr);
  //Serial.print(F("freeMemory()="));
  //Serial.println(freeMemory());
  char buffer[65];
  uint8_t nplHour = getTimeVal(NPLHOUR) - 1;
  uint8_t nplMinute = getTimeVal(NPLMINUTE);
  uint8_t nplDay = getTimeVal(NPLDAY);
  uint8_t nplMonth = getTimeVal(NPLMONTH);
  int nplYear = getTimeVal(NPLYEAR);
  // $--ZDA,hhmmss.ss,xx,xx,xxxx,xx,xx*hh<CR><LF>
  
  sprintf(buffer, " %02d/%02d/20%02d %02d:%02d:%02d ", getTimeVal(NPLDAY),
                                      getTimeVal(NPLMONTH),
                                      getTimeVal(NPLYEAR),
                                      getTimeVal(NPLHOUR),
                                      getTimeVal(NPLMINUTE),
                                      secondOffset                            
  );
 // Serial.println(buffer);
  

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
   sprintf(buffer, "$GPRMC,%02d%02d%02d,%c,4916.45,N,12311.12,W,000.5,054.7,%02d%02d%02d,020.3,E*",
                     nplHour,
                     nplMinute,
                     secondOffset,
                     NMEA_NAVIGATION_WARNING,
                     nplDay,
                     nplMonth,
                     nplYear);
  sprintf(buffer,"%s%02x",buffer,nmeaChecksum(buffer));
  Serial.println(buffer);
                     
 
  //Serial.println(nmeaChecksum(buffer));
 // D1088BRG.writeToDisplay(buffer,sizeof(buffer));
 Serial.flush();
  
}

int getTimeVal(struct timeElement element) {
    return bcdToDec(GetChunk(element.offset,element.numBytes,element.buffer));  
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
  uint8_t counter = numBits - 1;
  uint8_t bitCounter = 0;				// a count of the number of "1" bits

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
  memset((byte*)&aBuffer[0], 0x00, sizeof(aBuffer));
  memset((byte*)&bBuffer[0], 0x00, sizeof(bBuffer));
}

void clearSavedBuffers() {
  memset(&savedaBuffer[0], 0x00, sizeof(savedaBuffer));
  memset(&savedbBuffer[0], 0x00, sizeof(savedbBuffer));
}

void saveBuffers() {
  memcpy(&savedaBuffer[0],(byte*)&aBuffer[0],sizeof(aBuffer));
  memcpy(&savedbBuffer[0],(byte*)&bBuffer[0],sizeof(bBuffer));
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

//D1088BRG.initialize();

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
  
  // set the LED with the ledState of the variable:
  digitalWrite(ledPin, ledState);
  
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
    
    TOS = false;
    
    
    
    if ( TOM ) {
#ifdef DEBUG
        Serial.println("TOM");
#endif 
        
        if (   isParityValid(NPLYEARPARITY)
            && isParityValid(NPLDAYPARITY)
            && isParityValid(NPLDOWPARITY)
            && isParityValid(NPLTIMEPARITY) ) {
              NMEA_NAVIGATION_WARNING = *GPS_LOCK;
              saveBuffers();
            } else {
              NMEA_NAVIGATION_WARNING = *GPS_NO_LOCK;
            }
            
        
        secondOffset = 0;
        clearBuffers();   
        TOM = false;   
     }
   //check_mem(); 
   printTime();
  }
  
 
//D1088BRG.update(); 
}


