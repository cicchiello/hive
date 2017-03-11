#include <beecnt.h>

#include <Arduino.h>

#include <str.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <strbuf.h>

BeeCounter::BeeCounter(const HiveConfig &config, const char *name,
		       const class RateProvider &rateProvider,
		       const class TimeProvider &timeProvider,
		       unsigned long now,
		       int _ploadPin, int _clockPin, int _dataPin,
		       Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex),
    mFirstRead(true), mNumBeesIn(0), mNumBeesOut(0),
    ploadPin(_ploadPin), clockPin(_clockPin), dataPin(_dataPin),
    mLastSampleTime(now), mNextMidnight(0)
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

    
    mNextMidnight = 1489161600; // HKG midnight 3/11/2017
    while (mNextMidnight < now) {
        mNextMidnight += 60*60*24;
    }
}


BeeCounter::~BeeCounter()
{
}


inline static void clkDelay()
{
  for (int i = 0; i < 40; i++)
    ;
}

inline static void halfClkDelay()
{
  for (int i = 0; i < 20; i++)
    ;
}

PulseGenConsumer *BeeCounter::getPulseGenConsumer()
{
    return this;
}

void BeeCounter::readReg() 
{
    // Trigger a parallel load to latch the state of the data lines, using direct reg manipulation
    // instead of the more portable but slower: 
    //    digitalWrite(ploadPin, LOW);
    //    digitalWrite(ploadPin, HIGH);
    const PinDescription &pload = g_APinDescription[ploadPin];    // cache descriptor
    PORT->Group[pload.ulPort].OUTCLR.reg = (1ul << pload.ulPin) ; // pull low
    clkDelay();
    PORT->Group[pload.ulPort].OUTSET.reg = (1ul << pload.ulPin) ; // pull high
    

    const PinDescription &clk = g_APinDescription[clockPin];  // cache descriptor
    PORT->Group[clk.ulPort].OUTCLR.reg = (1ul << clk.ulPin) ; // start low
    
    const PinDescription &data = g_APinDescription[dataPin]; // cache descriptor; prepare to read
    
    // Loop to read each bit value from the serial out line of the SN74HC165N
    unsigned char byteVal = 0, bit = 1, i = 0;
    bool bitVal = false;
    for (int b = 0; b < NUM_BYTES; b++) {
        i = byteVal = 0;
	bit = 0x80;
	for (; i < 8; i++) {
	    bitVal = PORT->Group[data.ulPort].IN.reg & (1ul << data.ulPin); // read data pin

	    // pulse the clock (rising edge shifts the next bit)
	    PORT->Group[clk.ulPort].OUTTGL.reg = (1ul << clk.ulPin) ; // set clk HIGH

	    // spend a bit of time processing...
	    
	    // set the corresponding bit in byteVal
	    if (bitVal)
	        byteVal |= bit;
	    bit >>= 1;

	    halfClkDelay(); // delay half as much as normal since just spent some time processing
	    
	    PORT->Group[clk.ulPort].OUTTGL.reg = (1ul << clk.ulPin) ; // set clk LOW
	}
	mBytes[b] = byteVal;
    }
}



#define debeebounce 40
#define stucktime 700

bool BeeCounter::sensorSample(Str *value)
{
    char buf[10];
    StrBuf inStr(itoa(mNumBeesIn, buf, 10));
    StrBuf outStr(itoa(mNumBeesOut, buf, 10));

    StrBuf result(inStr);
    result.append("/");
    result.append(outStr.c_str());

    *value = result.c_str();

    return true;
}


void BeeCounter::pulse(unsigned long now)
{
    sample(now);
}


bool BeeCounter::sample(unsigned long now)
{
    TF("BeeCounter::pulse");

    if (now > mNextMidnight) {
        mNextMidnight += 60*60*24;
	mNumBeesIn = mNumBeesOut = 0;
    }
    
    if (now <= mLastSampleTime)
        return false;

    bool tookTooLong = false;
    if (!mFirstRead && (now > mLastSampleTime+10)) {
        TRACE2("now: ", now);
        PH3("BeeCounter::pulse called too infrequently; last call was : ", (now-mLastSampleTime),
	    " ms ago");
	tookTooLong = true;
    }
    mLastSampleTime = now;

    readReg();
  
    if (mFirstRead) {
        TRACE2("First read; now: ", now);
	mFirstRead = false;
	
	for (int i = 0; i < NUM_BYTES; i++)
	    mOldBytes[i] = mBytes[i];
  
    } else {
        for (int b = 0; b < NUM_BYTES; b++) {
	    if (mBytes[b] != mOldBytes[b]) {
	        D("found change on byte "); D(b); D(" byte="); DL(mBytes[b]);
	        for (unsigned char s = 0; s < 4; s++) {
		    unsigned char g = (b<<2) + s;
		    unsigned char nstate = (mBytes[b]>>(2*s))&0x03;
		
		    unsigned char outState = nstate & 1 ? 1 : 0;
		    unsigned char inState = nstate & 2 ? 1 : 0;
		
		    if (inState != mPrevIn[g]) {
		        if (inState == 0) {
		            D("bee just moved out from under IN sensor ");
			    DL(g);
			} else {
		            D("bee just arrived under IN sensor ");
			    DL(g);
			}
			if (now - mInTime[g] > debeebounce) {
		            if (inState == 0) {
			        mPrevInTime[g] = now;
				mInDuration[g] = now - mInTime[g];
				D("bee was under IN sensor ");
				D(g);
				D(" for ");
				D(mInDuration[g]);
				DL("ms");
				D("now - mPrevOutTime[g]: ");
				DL(now-mPrevOutTime[g]);
			    }
			    if (inState == 0 && outState == 0) {
			        // a bee just left the sensor
			        DL("outState is still 0");
				D("now - mPrevOutTime[g]: ");
				DL(now-mPrevOutTime[g]);
				if (now - mPrevOutTime[g] < 150) {
				    // indicates the leading edge of the outside sensore was just triggered ...
			            // indicating the bee movement is in the out direction
			            D("mInDuration[g]: ");
				    D(mInDuration[g]);
				    DL("ms");
				    D("mOutDuration[g]: ");
				    D(mOutDuration[g]);
				    DL("ms");
				    if (mInDuration[g] < stucktime || mOutDuration[g] < stucktime) {
				        // we got here, a bee has left the hive
				        D("A bee left the hive on gate ");
					DL(g);
					DL();
					mNumBeesOut++;
				    }
				}
			    }
			} else {
		            D("ignored due to debeebounce rule: t == ");
			    DL(now - mInTime[g]);
			}
			mInTime[g] = now;
			mPrevIn[g] = inState;
		    }
		    if (outState != mPrevOut[g]) {
		        if (outState == 0) {
		            D("bee just moved out from under OUT sensor ");
			    DL(g);
			} else {
		            D("bee just arrived under OUT sensor ");
			    DL(g);
			}
			if (now - mOutTime[g] > debeebounce) {
		            if (outState == 0) {
			        mPrevOutTime[g] = now;
				mOutDuration[g] = now - mOutTime[g];
				D("bee was under OUT sensor ");
				D(g);
				D(" for ");
				D(mOutDuration[g]);
				DL("ms");
				D("now - mPrevInTime[g]: ");
				DL(now-mPrevInTime[g]);
			    }
			    if (outState == 0 && inState == 0) {
			        // a bee just left the sensor
			        DL("inState is still 0");
				D("now - mPrevInTime[g]: ");
				DL(now-mPrevInTime[g]);
				if (now - mPrevInTime[g] < 150) {
				    // indicates the leading edge of the outside sensore was just triggered ...
			            // indicating the bee movement is in the in direction
			            D("mInDuration[g]: ");
				    D(mInDuration[g]);
				    DL("ms");
				    D("mOutDuration[g]: ");
				    D(mOutDuration[g]);
				    DL("ms");
				    if (mInDuration[g] < stucktime || mOutDuration[g] < stucktime) {
				        // we got here, a bee has arrived
				        D("A bee arrived in the hive on gate ");
					DL(g);
					DL();
					mNumBeesIn++;
				    }
				}
			    }
			} else {
		            D("ignored due to debeebounce rule: t == ");
			    DL(now - mOutTime[g]);
			}
			mOutTime[g] = now;
			mPrevOut[g] = outState;
		    }
		}
	    }
	    mOldBytes[b] = mBytes[b];
	}
    }

    return tookTooLong;
}
