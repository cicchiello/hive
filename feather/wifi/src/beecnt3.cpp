#include <Arduino.h>

#include <beecnt3.h>

#include <str.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>


#define SAMPLE_INIT           1
#define SAMPLE_LOAD_LOW       2
#define SAMPLE_LOAD_HIGH      3
#define SAMPLE_CLK_INIT       4
#define SAMPLE_CLK_HIGH_IN    5
#define SAMPLE_CLK_LOW_IN     6
#define SAMPLE_CLK_HIGH_OUT   7
#define SAMPLE_CLK_LOW_OUT    8
#define SAMPLE_BIT_PROCESSING 9
#define SAMPLE_IDLE           10
#define SAMPLE_ERROR          11

#define DEBEEBOUNCE 40
#define STUCKTIME 700


#define QUEUE_DEPTH  750
static volatile BeeCounter3::Sample sValues[QUEUE_DEPTH];
static volatile BeeCounter3::Sample *sCurrValue;
static volatile BeeCounter3::Sample *sHeadValue;
static volatile BeeCounter3::Sample *sTailValue;

#define TIMING_DIAGNOSTICS
#ifdef TIMING_DIAGNOSTICS
static volatile int sNumSamplesTaken = 0;
static volatile unsigned long sConsumptionTime_us = 0;
static unsigned long sLongestLoopDelta_ms = 0;
static unsigned long sLastLoopTime_ms = 0;
static unsigned long sPrevSampleTime_us = 0;
static unsigned long sLongestSampleDelta_us = 0;
#endif


BeeCounter3::BeeCounter3(const HiveConfig &config, const char *name,
			 const class RateProvider &rateProvider,
			 const class TimeProvider &timeProvider,
			 unsigned long now,
			 int ploadPin, int clockPin, int dataPin,
			 Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex),
    mFirstRead(true), mNumBees(0), mNextActionTime(now+1000l),
    mLoad(g_APinDescription[ploadPin]), // cache LOAD pin descriptor
    mClk(g_APinDescription[clockPin]), // cache CLK pin descriptor
    mData(g_APinDescription[dataPin]), // cache DATA pin descriptor
    mLoadReg(PORT->Group[g_APinDescription[ploadPin].ulPort]), // cache LOAD register struct
    mClkReg(PORT->Group[g_APinDescription[clockPin].ulPort]), // cache CLK register struct
    mDataReg(PORT->Group[g_APinDescription[dataPin].ulPort]) // cache DATA register struct
{
    TF("BeeCounter3::BeeCounter3");
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
    
    for (int i = 0; i < NUM_GATES; i++) {
	mInTime[i] = mPrevInTime[i] = mOutTime[i] = mPrevOutTime[i] = 0;
	mInDuration[i] = mOutDuration[i] = 0;
	mPrevOutState[i] = mPrevInState[i] = false;
    }

    mReadState = SAMPLE_IDLE;
//    HivePlatform::nonConstSingleton()->registerPulseGenConsumer_44K(this);

    for (int i = 0; i < QUEUE_DEPTH-1; i++) {
        sValues[i].next = &sValues[i+1];
    }
    sValues[QUEUE_DEPTH-1].next = NULL;
    sTailValue = &sValues[QUEUE_DEPTH-1];
    sHeadValue = sCurrValue = &sValues[0];
    
    setNextSampleTime(now + 1000l);
}


BeeCounter3::~BeeCounter3()
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


void BeeCounter3::pulse(unsigned long now)
{
    TF("BeeCounter3::pulse");
//    interrupts();
    
    switch (mReadState) {
    case SAMPLE_ERROR:
    case SAMPLE_IDLE: {
        // no-op
    }
      break;
    case SAMPLE_INIT:
#ifdef TIMING_DIAGNOSTICS      
        sPrevSampleTime_us = micros();
#endif
	// intentionally continues into the SAMPLE_LOAD_LOW state 
	
    case SAMPLE_LOAD_LOW: {
        // Trigger a parallel load to latch the state of the data lines, using direct reg manipulation
        // instead of the more portable but slower: 
        //    digitalWrite(ploadPin, LOW);
        mLoadReg.OUTCLR.reg = mLoadMask; // pull low
	mReadState = SAMPLE_LOAD_HIGH;

#ifdef TIMING_DIAGNOSTICS	
	sNumSamplesTaken++;
	unsigned long now_us = micros();
	unsigned long delta_us = now_us - sPrevSampleTime_us;
	sConsumptionTime_us += delta_us;
	if (delta_us > sLongestSampleDelta_us)
	    sLongestSampleDelta_us = delta_us;
	sPrevSampleTime_us = now_us;
#endif	
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
	mReadState = SAMPLE_CLK_HIGH_IN;
	mGate = NUM_GATES-1;
    }
      break;
    case SAMPLE_CLK_HIGH_IN: {
        mInState = mDataReg.IN.reg & mDataMask; // read data pin

	// pulse the clock (rising edge shifts the next bit)
	mClkReg.OUTTGL.reg = mClkMask ; // set clk HIGH
	
	mReadState = SAMPLE_CLK_LOW_IN;
    }
      break;
    case SAMPLE_CLK_LOW_IN: {
	mClkReg.OUTTGL.reg = mClkMask ; // set clk LOW
	mReadState = SAMPLE_CLK_HIGH_OUT;
    }
      break;
    case SAMPLE_CLK_HIGH_OUT: {
        mOutState = mDataReg.IN.reg & mDataMask; // read data pin

	// pulse the clock (rising edge shifts the next bit)
	mClkReg.OUTTGL.reg = mClkMask ; // set clk HIGH
	
	mReadState = SAMPLE_CLK_LOW_OUT;
    }
      break;
    case SAMPLE_CLK_LOW_OUT: {
	mClkReg.OUTTGL.reg = mClkMask ; // set clk LOW
	mReadState = SAMPLE_BIT_PROCESSING;

	// The following block takes about 3us worst case!
	// do half of the bit processing now, and the other half on the next ISR pulse
	if (mInState != mPrevInState[mGate]) {
	    unsigned long delta = now-mInTime[mGate];
	    if ((delta > DEBEEBOUNCE) && mInState) {
	        mPrevInTime[mGate] = now;
		mInDuration[mGate] = delta;
		if (mOutState && (now - mPrevOutTime[mGate] < 150)) {
		    // a bee just left the sensor, and...
		    // indicates the leading edge of the outside sensor was just triggered ...
		    // indicating the bee movement is in the out direction
		    if (delta < STUCKTIME || mOutDuration[mGate] < STUCKTIME) {
		        // if we got here, a bee has left the hive
		        mNumBees--;
		    }
		}
	    }
	    mInTime[mGate] = now;
	    mPrevInState[mGate] = mInState;
	}

	//to see the pulse on the scope, enable "5" as an output and uncomment:
	//const PinDescription &p = g_APinDescription[5];
	//PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
	//PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
    }
      break;
    case SAMPLE_BIT_PROCESSING: {
	// doing second half of the bit processing 
	if (mOutState != mPrevOutState[mGate]) {
	    unsigned long delta = now-mOutTime[mGate];
	    if ((delta > DEBEEBOUNCE) && mOutState) {
	        mPrevOutTime[mGate] = now;
		mOutDuration[mGate] = delta;

		if (mInState && (now - mPrevInTime[mGate] < 150)) {
		    // a bee just left the sensor, and 
		    // indicates the leading edge of the outside sensor was just triggered ...
		    // indicating the bee movement is in the in direction
		    if (mInDuration[mGate] < STUCKTIME || delta < STUCKTIME) {
		        // we got here, a bee has arrived
		        mNumBees++;
		    }
		}
	    }
	    mOutTime[mGate] = now;
	    mPrevOutState[mGate] = mOutState;
	}

	// setup for next bit, or start of next sample
	mReadState = mGate-- == 0 ? SAMPLE_LOAD_LOW : SAMPLE_CLK_HIGH_IN;
    }
      break;
    default:
      assert2(false, "Invalid state: ", mReadState);
    }
}


static int sProcessCnt = 0;
void BeeCounter3::processSampleQueue()
{
    TF("BeeCounter3::processSampleQueue");
    sProcessCnt++;
    if (mReadState == SAMPLE_ERROR) {
        TRACE("About to fire assertion");
	TRACE2("sNumSamplesTaken: ", sNumSamplesTaken);
	TRACE2("sConsumptionTime_us: ", sConsumptionTime_us);
	TRACE2("avg consumption time (us): ", (sConsumptionTime_us/sNumSamplesTaken));
	TRACE2("sProcessCnt: ", sProcessCnt);
	TRACE2("sizeof(unsigned long): ", sizeof(unsigned long));
	TRACE2("sizeof(Sample*): ", sizeof(Sample*));
	TRACE2("sizeof(struct Sample): ", sizeof(struct Sample));
	TRACE2("sLongestSampleDelta_us: ", sLongestSampleDelta_us);
	assert(false, "forced stop");
    } else {
        while (sCurrValue != sHeadValue) {
	    // careful about how you pop a value off the queue -- there's a background ISR that may update it at any time
	    volatile Sample *popped = sHeadValue;
	    sHeadValue = popped->next;
	    processSample(*popped);
	    popped->next = NULL;
	    sTailValue->next = popped;
	    sTailValue = popped;
	}
    }
}




void BeeCounter3::processSample(const volatile Sample &sample)
{
    TF("BeeCounter3::processSample");
    
#ifdef TIMING_DIAGNOSTICS    
    unsigned long consumeStart = micros();
#endif
    
    if (mFirstRead) {
	mFirstRead = false;

	mPrevSampleTimestamp = millis();
	mPrevInReading = sample.inReading;
	mPrevOutReading = sample.outReading;
    } else {
        unsigned long now = sample.delta_ms + mPrevSampleTimestamp;
	mPrevSampleTimestamp = now;
	if ((sample.inReading != mPrevInReading) || (sample.outReading != mPrevOutReading)) {
	    TRACE4("found change on reading; in: ", sample.inReading, " out: ", sample.outReading);
	    for (unsigned char g = 0, mask = 1; g < NUM_GATES; g++) {
	        bool outState = !(sample.outReading & mask);
		bool inState = !(sample.inReading & mask);
		bool pOutState = !(mPrevOutReading & mask);
		bool pInState = !(mPrevInReading & mask);
		mask <<= 1;
		if (inState != pInState) {
		    TRACE4("bee just ", (inState == 0 ? "moved out from" : "arrived"), " under IN sensor ", g);
		    unsigned long delta = now-mInTime[g];
		    if (delta > DEBEEBOUNCE) {
		        if (inState) {
			    mPrevInTime[g] = now;
			    mInDuration[g] = delta;
			    TRACE4("bee was under IN sensor ", g, " for (ms) ", mInDuration[g]);
			    TRACE2("now - mPrevOutTime[g]: ", (now-mPrevOutTime[g]));
			    
			    if (outState) {
			        // a bee just left the sensor
			        TRACE("outState is still 0");
				TRACE2("now - mPrevOutTime[g]: ", (now-mPrevOutTime[g]));
				if (now - mPrevOutTime[g] < 150) {
				    // indicates the leading edge of the outside sensor was just triggered ...
				    // indicating the bee movement is in the out direction
				    TRACE3("mInDuration[g]: ", mInDuration[g], " ms");
				    TRACE3("mOutDuration[g]: ", mOutDuration[g], " ms");
				    if (mInDuration[g] < STUCKTIME || mOutDuration[g] < STUCKTIME) {
				        // if we got here, a bee has left the hive
				        TRACE2("A bee left the hive on gate ", g);
					TRACE("");
					mNumBees--;
				    }
				}
			    }
			}
		    } else {
		        TRACE3("ignored due to DEBEEBOUNCE rule: t == ", delta, " ms");
		    }
		    mInTime[g] = now;
		}
		if (outState != pOutState) {
		    TRACE4("bee just ", (outState == 0 ? "moved out from" : "arrived"), " under OUT sensor ", g);
		    unsigned long delta = now-mOutTime[g];
		    if (delta > DEBEEBOUNCE) {
		        if (outState) {
			    mPrevOutTime[g] = now;
			    mOutDuration[g] = delta;
			    TRACE4("bee was under OUT sensor ", g, " for (ms) ", mOutDuration[g]);
			    TRACE2("now - mPrevInTime[g]: ", (now-mPrevInTime[g]));

			    if (inState) {
			        // a bee just left the sensor
			        TRACE("inState is still 0");
				TRACE2("now - mPrevInTime[g]: ", (now-mPrevInTime[g]));
				if (now - mPrevInTime[g] < 150) {
				    // indicates the leading edge of the outside sensor was just triggered ...
				    // indicating the bee movement is in the in direction
				    TRACE3("mInDuration[g]: ", mInDuration[g], " ms");
				    TRACE3("mOutDuration[g]: ", mOutDuration[g], " ms");
				    if (mInDuration[g] < STUCKTIME || mOutDuration[g] < STUCKTIME) {
				        // we got here, a bee has arrived
				        TRACE2("A bee arrived in the hive on gate ", g);
					TRACE("");
					mNumBees++;
				    }
				}
			    }
			}
		    } else {
		        TRACE3("ignored due to DEBEEBOUNCE rule: t == ", delta, " ms");
		    }
		    mOutTime[g] = now;
		}
	    }
	  
	    mPrevInReading = sample.inReading;
	    mPrevOutReading = sample.outReading;
	}
    }

#ifdef TIMING_DIAGNOSTICS    
    sConsumptionTime_us += (micros()-consumeStart);
#endif    
}



bool BeeCounter3::sensorSample(Str *value)
{
    char buf[10];
    *value = itoa(mNumBees, buf, 10);

    return true;
}


bool BeeCounter3::isItTimeYet(unsigned long now)
{
    return now >= mNextActionTime || SensorBase::isItTimeYet(now);
}


static long sLoopCnt = 0;
static unsigned long sTotalLoopTime_ms = 0;
bool BeeCounter3::loop(unsigned long now)
{
    TF("BeeCounter3::loop");
    //TRACE2("entry: ", now);

    ++sLoopCnt;
    if (mReadState == SAMPLE_IDLE) {
        TRACE2("Enabling BeeCounter hardware readings; now: ", now);
        mReadState = SAMPLE_INIT;
    } else if (sLoopCnt % 1000 == 0) {
	TRACE2("sNumSamplesTaken: ", sNumSamplesTaken);
	TRACE2("sConsumptionTime_us: ", sConsumptionTime_us);
	TRACE2("avg consumption time (us): ", (sConsumptionTime_us/sNumSamplesTaken));
	TRACE2("sLongestLoopDelta_ms: ", sLongestLoopDelta_ms);
	TRACE2("avg(loopDelta): ", (sTotalLoopTime_ms/sLoopCnt));
	TRACE2("sLongestSampleDelta_us: ", sLongestSampleDelta_us);

        unsigned long loopDelta_ms = now - sLastLoopTime_ms;
	sTotalLoopTime_ms += loopDelta_ms;
	if (loopDelta_ms > sLongestLoopDelta_ms) {
	    TRACE2("Found longer loopDelta: ", loopDelta_ms);
	    sLongestLoopDelta_ms = loopDelta_ms;
	}
    }
    sLastLoopTime_ms = now;
    
//    processSampleQueue();

    now = millis();
    mNextActionTime = now+5l;

    //TRACE("forwarding control to SensorBase::loop");
    return SensorBase::loop(now);
}
