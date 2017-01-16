#include <beecnt.h>

#include <Arduino.h>

#include <str.h>


#define HEADLESS
#define NDEBUG


#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#ifndef NDEBUG
#   define NDEBUG
#endif
#define P(args) do {} while (0)
#define PL(args) do {} while (0)
#endif

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#ifndef NDEBUG
#define assert(c,msg) if (!(c)) {PL("ASSERT"); WDT_TRACE(msg); while(1);}
#else
#define assert(c,msg) do {} while(0);
#endif

static const char *sFunc = "<undefinedFunc>";
class TraceScope {
  const char *prev;
public:
  TraceScope(const char *func) : prev(sFunc) {sFunc = func;}
  ~TraceScope() {sFunc = prev;}
};

//#define TF(f) TraceScope tscope(f);
//#define TRACE(msg) {P("TRACE: "); P(sFunc); P(" ;"); PL(msg);}
#define TF(f) do {} while (0);
#define TRACE(msg) do {} while (0);


#define PULSE_WIDTH_USEC   5

BeeCounter::BeeCounter(const char *name, const class RateProvider &rateProvider, unsigned long now,
		       int _ploadPin, int _clockPin, int _dataPin)
  : Sensor(name, rateProvider, now),
    mFirstRead(true), mNumBees(0),
    ploadPin(_ploadPin), clockPin(_clockPin), dataPin(_dataPin)
{
    //initialize digital pins
    pinMode(ploadPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, INPUT);
    
    digitalWrite(clockPin, LOW);
    digitalWrite(ploadPin, HIGH);

    for (int i = 0; i < NUM_BYTES; i++)
        mBytes[i] = mOldBytes[i] = 0;
    for (int i = 0; i < NUM_GATES; i++) {
        mPrevIn[i] = mPrevOut[i] = 0;
	mInTime[i] = mPrevInTime[i] = mOutTime[i] = mPrevOutTime[i] = 0;
	mInDuration[i] = mOutDuration[i] = 0;
    }
}


BeeCounter::~BeeCounter()
{
}

void BeeCounter::readReg() 
{
  TF("BeeCounter::readReg()");
  TRACE("0");
    // Trigger a parallel load to latch the state of the data lines
    digitalWrite(ploadPin, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(ploadPin, HIGH);

  TRACE("1");
    // Loop to read each bit value from the serial out line of the SN74HC165N
    for (int b = 0; b < NUM_BYTES; b++) {
        unsigned char byteVal = 0;
	for (int i = 0; i < 8; i++) {
	    long bitVal = digitalRead(dataPin);

	    // set the corresponding bit in byteVal
	    byteVal |= (bitVal << ((8-1) - i));

	    // pulse the clock (rising edge shifts the next bit)
	    digitalWrite(clockPin, HIGH);
	    delayMicroseconds(PULSE_WIDTH_USEC);
	    digitalWrite(clockPin, LOW);
	}
	mBytes[b] = byteVal;
    }
  TRACE("2");
  
}


bool BeeCounter::isItTimeYet(unsigned long now)
{
    return true;
}


void BeeCounter::display_pin_values() const
{
    P("Pin states: ");
    bool allZeroes = true;
    for (int i = 0; i < NUM_BYTES; i++) {
        P("0b"); Serial.print(mBytes[i], BIN); P(" ");
	if (mBytes[i]) allZeroes = false;
    }
    PL();
    if (allZeroes)
        PL();
}


#define debeebounce 50
#define stucktime 700

bool BeeCounter::sensorSample(Str *value)
{
  TF("BeeCounter::sensorSample(Str*)");
  TRACE("0");
    readReg();
  
  TRACE("1");
    unsigned long now = millis();
    bool foundDiff = false;
    if (mFirstRead) {
        foundDiff = true;  // treat the first sample as a change
	mFirstRead = false;
    } else {
  TRACE("2");
        for (int i = 0; !foundDiff && (i < NUM_BYTES); i++) 
	    foundDiff = mBytes[i] != mOldBytes[i];
  TRACE("3");
	if (foundDiff) {
  TRACE("4");
	    for (int i = 0; i < NUM_GATES; i++) {
  TRACE("5");
	        int b = i/4;
		int s = i % 4;
		int nstate = (mBytes[b]>>(2*s))&0x03;
		int ostate = (mOldBytes[b]>>(2*s))&0x03;
		
		unsigned char outState = nstate & 1 ? 1 : 0;
		unsigned char inState = nstate & 2 ? 1 : 0;
		
		if (inState != mPrevIn[i]) {
		    if (inState == 0) {
		        D("bee just left IN sensor ");
			DL(i);
		    } else {
		        D("bee just arrived under IN sensor ");
			DL(i);
		    }
		    if (now - mInTime[i] > debeebounce) {
		        if (inState == 0) {
			    mPrevInTime[i] = now;
			    mInDuration[i] = now - mInTime[i];
			    D("bee was under IN sensor ");
			    D(i);
			    D(" for ");
			    D(mInDuration[i]);
			    DL("ms");
			    D("now - mPrevOutTime[i]: ");
			    DL(now-mPrevOutTime[i]);
			}
			if (inState == 0 && outState == 0) {
			    // a bee just left the sensor
			    DL("outState is still 0");
			    D("now - mPrevOutTime[i]: ");
			    DL(now-mPrevOutTime[i]);
			    if (now - mPrevOutTime[i] < 150) {
			        // indicates the leading edge of the outside sensore was just triggered ...
			        // indicating the bee movement is in the out direction
			        D("mInDuration[i]: ");
				D(mInDuration[i]);
				DL("ms");
				D("mOutDuration[i]: ");
				D(mOutDuration[i]);
				DL("ms");
				if (mInDuration[i] < stucktime || mOutDuration[i] < stucktime) {
				    // we got here, a bee has left
				    P("A bee left on gate ");
				    PL(i);
				    PL();
				    mNumBees--;
				}
			    }
			}
		    } else {
		        D("ignored due to debeebounce rule: t == ");
			DL(now - mInTime[i]);
		    }
		    mInTime[i] = now;
		    mPrevIn[i] = inState;
		}
		if (outState != mPrevOut[i]) {
		    if (outState == 0) {
		        D("bee just left OUT sensor ");
			DL(i);
		    } else {
		        D("bee just arrived under OUT sensor ");
			DL(i);
		    }
		    if (now - mOutTime[i] > debeebounce) {
		        if (outState == 0) {
			    mPrevOutTime[i] = now;
			    mOutDuration[i] = now - mOutTime[i];
			    D("bee was under OUT sensor ");
			    D(i);
			    D(" for ");
			    D(mOutDuration[i]);
			    DL("ms");
			    D("now - mPrevInTime[i]: ");
			    DL(now-mPrevInTime[i]);
			}
			if (outState == 0 && inState == 0) {
			    // a bee just left the sensor
			    DL("inState is still 0");
			    D("now - mPrevInTime[i]: ");
			    DL(now-mPrevInTime[i]);
			    if (now - mPrevInTime[i] < 150) {
			        // indicates the leading edge of the outside sensore was just triggered ...
			        // indicating the bee movement is in the in direction
			        D("mInDuration[i]: ");
				D(mInDuration[i]);
				DL("ms");
				D("mOutDuration[i]: ");
				D(mOutDuration[i]);
				DL("ms");
				if (mInDuration[i] < stucktime || mOutDuration[i] < stucktime) {
				    // we got here, a bee has arrived
				    P("A bee arrived on gate ");
				    PL(i);
				    PL();
				    mNumBees++;
				}
			    }
			}
		    } else {
		        D("ignored due to debeebounce rule: t == ");
			DL(now - mOutTime[i]);
		    }
		    mOutTime[i] = now;
		    mPrevOut[i] = outState;
		}
	    }
	}
    }

    if (foundDiff) {
        for (int i = 0; i < NUM_BYTES; i++)
	    mOldBytes[i] = mBytes[i];
    }
  
    char buf[10];
    *value = itoa(mNumBees, buf, 10);

    return true;
}


