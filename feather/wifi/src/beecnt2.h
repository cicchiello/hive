#ifndef BeeCounter2_h
#define BeeCounter2_h


#include <SensorBase.h>
#include <pulsegen_consumer.h>


class BeeCounter2 : public SensorBase, private PulseGenConsumer {
 public:
    BeeCounter2(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       const class TimeProvider &timeProvider,
	       unsigned long now,
	       int ploadPin, int clockPin, int dataPin,
	       Mutex *wifiMutex);
    ~BeeCounter2();

    bool sensorSample(Str *value);
    
    bool loop(unsigned long now);
    

    static const int NUM_BYTES = 3;
    static const int NUM_GATES = NUM_BYTES*4;
    
    struct Sample {
        unsigned char bytes[NUM_BYTES];
        unsigned char delta_ms;
        volatile Sample *next;
    };
    
 private:
    const char *className() const {return "BeeCounter";}
    
    bool isItTimeYet(unsigned long now);
    
    void pulse(unsigned long now);
    
    void processSample(const volatile Sample &reading);
    void processSampleQueue();

    int mNumBees;
    

    bool mFirstRead;
    unsigned char mPrevBytes[NUM_BYTES], mPrevIn[NUM_GATES], mPrevOut[NUM_GATES];
    unsigned long mInTime[NUM_GATES], mPrevInTime[NUM_GATES], mOutTime[NUM_GATES], mPrevOutTime[NUM_GATES];
    unsigned long mInDuration[NUM_GATES], mOutDuration[NUM_GATES], mNextActionTime, mPrevSampleTimestamp;
    unsigned char mReadState, mReadByte, mReadBit, mByteVal, mBitMask;

    // cache a bunch of hardware mappings to speed up ISRXS
    const PinDescription &mLoad, &mClk, &mData;
    PortGroup &mLoadReg, &mClkReg, &mDataReg;
    unsigned int mLoadMask, mClkMask, mDataMask;
    
    int dataPin;
};


#endif
