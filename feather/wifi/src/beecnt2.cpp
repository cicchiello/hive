#include <Arduino.h>

#include <beecnt2.h>

#include <str.h>


//#define HEADLESS
//#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>


#define SAMPLE_INIT      1
#define SAMPLE_LOAD_LOW  2
#define SAMPLE_LOAD_HIGH 3
#define SAMPLE_CLK_INIT  4
#define SAMPLE_CLK_HIGH  5
#define SAMPLE_CLK_LOW   6
#define SAMPLE_IDLE      7
#define SAMPLE_ERROR     8

#define SAMPLE_LOOP      SAMPLE_LOAD_LOW


#define QUEUE_DEPTH  750
static volatile BeeCounter2::Sample sValues[QUEUE_DEPTH];
static volatile BeeCounter2::Sample *sCurrValue;
static volatile BeeCounter2::Sample *sHeadValue;
static volatile BeeCounter2::Sample *sTailValue;

#define TIMING_DIAGNOSTICS
#ifdef TIMING_DIAGNOSTICS
static volatile int sNumSamplesTaken = 0;
static volatile int sNumSamplesConsumed = 0;
static volatile unsigned long sConsumptionTime_us = 0;
static unsigned long sLongestRefreshTime = 0;
static unsigned long sLastRefreshTime = 0;
static unsigned long sPrevSampleTime = 0;
static unsigned long sLongestSampleDelta = 0;
#endif


BeeCounter2::BeeCounter2(const HiveConfig &config, const char *name,
			 const class RateProvider &rateProvider,
			 const class TimeProvider &timeProvider,
			 unsigned long now,
			 int ploadPin, int clockPin, int dataPin,
			 Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex),
    mFirstRead(true), mNumBees(0),
    mLoad(g_APinDescription[ploadPin]), // cache LOAD pin descriptor
    mClk(g_APinDescription[clockPin]), // cache CLK pin descriptor
    mData(g_APinDescription[dataPin]), // cache DATA pin descriptor
    mLoadReg(PORT->Group[g_APinDescription[ploadPin].ulPort]), // cache LOAD register struct
    mClkReg(PORT->Group[g_APinDescription[clockPin].ulPort]), // cache CLK register struct
    mDataReg(PORT->Group[g_APinDescription[dataPin].ulPort]) // cache DATA register struct
{
    TF("BeeCounter2::BeeCounter2");
    TRACE("entry");
    
    //initialize digital pins
    pinMode(ploadPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, INPUT);
    
    digitalWrite(clockPin, LOW);
    digitalWrite(ploadPin, HIGH);

    mLoadMask = 1ul << mLoad.ulPin;
    mClkMask = 1ul << mClk.ulPin;
    mDataMask = 1ul << mData.ulPin;
    
    for (int i = 0; i < NUM_BYTES; i++)
        mPrevBytes[i] = 0;
    for (int i = 0; i < NUM_GATES; i++) {
        mPrevIn[i] = mPrevOut[i] = 0;
	mInTime[i] = mPrevInTime[i] = mOutTime[i] = mPrevOutTime[i] = 0;
	mInDuration[i] = mOutDuration[i] = 0;
    }

    mReadState = SAMPLE_IDLE;
    HivePlatform::nonConstSingleton()->registerPulseGenConsumer_44K(this);

    for (int i = 0; i < QUEUE_DEPTH-1; i++) {
        sValues[i].next = &sValues[i+1];
    }
    sValues[QUEUE_DEPTH-1].next = NULL;
    sTailValue = &sValues[QUEUE_DEPTH-1];
    sHeadValue = sCurrValue = &sValues[0];
    
    setNextSampleTime(now + 1000l);
}


BeeCounter2::~BeeCounter2()
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




void BeeCounter2::pulse(unsigned long now)
{
    TF("BeeCounter2::pulse");
    switch (mReadState) {
    case SAMPLE_ERROR:
    case SAMPLE_IDLE: {
        // no-op
    }
      break;
//    case SAMPLE_INIT: {
//	for (int i = 0; i < QUEUE_DEPTH-1; i++) {
//	    sValues[i].next = &sValues[i+1];
//	}
//	sValues[QUEUE_DEPTH-1].next = NULL;
//	sTailValue = &sValues[QUEUE_DEPTH-1];
//        sHeadValue = sCurrValue = &sValues[0];
//	
//	mReadState = SAMPLE_LOAD_LOW;
//    }
//      break;
    case SAMPLE_INIT:
        sPrevSampleTime = now;
	
    case SAMPLE_LOAD_LOW: {
        // Trigger a parallel load to latch the state of the data lines, using direct reg manipulation
        // instead of the more portable but slower: 
        //    digitalWrite(ploadPin, LOW);
        mLoadReg.OUTCLR.reg = mLoadMask; // pull low
	mReadState = SAMPLE_LOAD_HIGH;
    }
      break;
    case SAMPLE_LOAD_HIGH: {
        // second part of triggering a parallel load to latch the state of the data lines
        //   i.e. digitalWrite(ploadPin, HIGH);
        mLoadReg.OUTSET.reg = mLoadMask; // pull high
	mReadState = SAMPLE_CLK_INIT;
    }
      break;
    case SAMPLE_CLK_INIT: {
        mClkReg.OUTCLR.reg = mClkMask; // start low
	mReadState = SAMPLE_CLK_HIGH;
	mReadByte = mReadBit = 0;
	mByteVal = 0;
	mBitMask = 0x80;
    }
      break;
    case SAMPLE_CLK_HIGH: {
        if (mDataReg.IN.reg & mDataMask) // read data pin
	    mByteVal |= mBitMask;
	
	// pulse the clock (rising edge shifts the next bit)
	mClkReg.OUTTGL.reg = mClkMask ; // set clk HIGH
	
	mBitMask >>= 1;
	mReadState = SAMPLE_CLK_LOW;
    }
      break;
    case SAMPLE_CLK_LOW: {
	mClkReg.OUTTGL.reg = mClkMask ; // set clk LOW
	mReadState = SAMPLE_CLK_HIGH;
	if (mBitMask == 0) {
	    sCurrValue->bytes[mReadByte++] = mByteVal;
	    if (mReadByte == NUM_BYTES) {
	        mReadState = SAMPLE_LOOP;

#ifdef TIMING_DIAGNOSTICS
		unsigned long sampleDelta = now-sPrevSampleTime;
		if ((sPrevSampleTime > 0) && (sampleDelta > sLongestSampleDelta))
		    sLongestSampleDelta = sampleDelta;
#endif

		assert((now-sPrevSampleTime) < 3, "(now-sPrevSampleTime) < 3");
		sCurrValue->delta_ms = (unsigned char) (now-sPrevSampleTime);
		sCurrValue = sCurrValue->next;
		sPrevSampleTime = now;
		
#ifdef TIMING_DIAGNOSTICS
		sNumSamplesTaken++;
		if (sCurrValue == NULL) {
		    mReadState = SAMPLE_ERROR;
		}
#endif
	    } else {
		mBitMask = 0x80;
		mByteVal = 0;
	    }
	}
    }
      break;
    default:
      assert2(false, "Invalid state: ", mReadState);
    }
}


static int sProcessCnt = 0;
void BeeCounter2::processSampleQueue()
{
    TF("BeeCounter2::processSampleQueue");
    sProcessCnt++;
    if (mReadState == SAMPLE_ERROR) {
        TRACE("About to fire assertion");
	TRACE2("sNumSamplesTaken: ", sNumSamplesTaken);
	TRACE2("sNumSamplesConsumed: ", sNumSamplesConsumed);
	TRACE2("sConsumptionTime_us: ", sConsumptionTime_us);
	TRACE2("avg consumption time (us): ", (sConsumptionTime_us/sNumSamplesConsumed));
	TRACE2("sProcessCnt: ", sProcessCnt);
	TRACE2("sLongestRefreshTime (ms): ", sLongestRefreshTime);
	TRACE2("sizeof(unsigned long): ", sizeof(unsigned long));
	TRACE2("sizeof(Sample*): ", sizeof(Sample*));
	TRACE2("sizeof(struct Sample): ", sizeof(struct Sample));
	TRACE2("sLongestSampleDelta: ", sLongestSampleDelta);
	assert(false, "forced stop");
    } else {
        while (sCurrValue != sHeadValue) {
	    // careful about how you pop a value off the queue -- there's a background ISR that may update it at any time
	    volatile Sample *popped = sHeadValue;
	    sHeadValue = popped->next;
	    sNumSamplesConsumed++;
	    processSample(*popped);
	    popped->next = NULL;
	    sTailValue->next = popped;
	    sTailValue = popped;
	}
    }
}



#define debeebounce 40
#define stucktime 700


void BeeCounter2::processSample(const volatile Sample &reading)
{
    TF("BeeCounter2::processSample");
    
#ifdef TIMING_DIAGNOSTICS    
    unsigned long consumeStart = micros();
#endif
    
    if (mFirstRead) {
	mFirstRead = false;

	mPrevSampleTimestamp = millis();
	for (int i = 0; i < NUM_BYTES; i++)
	    mPrevBytes[i] = reading.bytes[i];
  
    } else {
        unsigned long now = reading.delta_ms + mPrevSampleTimestamp;
	mPrevSampleTimestamp = now;
        for (int b = 0; b < NUM_BYTES; b++) {
	    unsigned char currByte = reading.bytes[b];
	    if (currByte != mPrevBytes[b]) {
	        D("found change on byte "); D(b); D(" byte="); DL(currByte);
	        for (unsigned char s = 0; s < 4; s++) {
		    unsigned char g = (b<<2) + s;
		    unsigned char nstate = (currByte>>(2*s))&0x03;
		
		    unsigned char outState = nstate & 1;
		    unsigned char inState = (nstate & 2) >> 1;
		
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
					mNumBees--;
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
					mNumBees++;
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
	    mPrevBytes[b] = currByte;
	}
    }

#ifdef TIMING_DIAGNOSTICS    
    sConsumptionTime_us += (micros()-consumeStart);
#endif    
}



bool BeeCounter2::sensorSample(Str *value)
{
    char buf[10];
    *value = itoa(mNumBees, buf, 10);

    return true;
}


bool BeeCounter2::isItTimeYet(unsigned long now)
{
    return now >= mNextActionTime || SensorBase::isItTimeYet(now);
}


bool BeeCounter2::loop(unsigned long now)
{
    TF("SensorBase::loop");

    if (mReadState == SAMPLE_IDLE) {
        TRACE2("Enabling BeeCounter hardware readings; now: ", now);
        mReadState = SAMPLE_INIT;
	sLastRefreshTime = now;
    } else {
        unsigned long refreshTime = now - sLastRefreshTime;
	if (refreshTime > sLongestRefreshTime) {
	    TRACE2("Found longer refreshTime: ", refreshTime);
	    sLongestRefreshTime = refreshTime;
	}
	sLastRefreshTime = now;
    }
    
    processSampleQueue();

    now = millis();
    mNextActionTime = now+5l;

    return SensorBase::loop(now);
}
