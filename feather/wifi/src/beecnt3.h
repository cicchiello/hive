#ifndef BeeCounter3_h
#define BeeCounter3_h


#include <SensorBase.h>
#include <pulsegen_consumer.h>


class BeeCounter3 : public SensorBase, private PulseGenConsumer {
 public:
    BeeCounter3(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       const class TimeProvider &timeProvider,
	       unsigned long now,
	       int ploadPin, int clockPin, int dataPin,
	       Mutex *wifiMutex);
    ~BeeCounter3();

    bool sensorSample(Str *value);
    
    bool loop(unsigned long now);
    

    static const int NUM_BYTES = 3;
    static const int NUM_GATES = NUM_BYTES*4;
    static const uint32_t IN_MASK = 0xaaaaaa;
    static const uint32_t OUT_MASK = 0x555555;
    
    struct Sample {
        uint32_t inReading;
        uint32_t outReading;
        unsigned char delta_ms;
        volatile Sample *next;
    };
    
    void pulse(unsigned long now);
    
 private:
    const char *className() const {return "BeeCounter";}
    
    bool isItTimeYet(unsigned long now);
    
    void processSample(const volatile Sample &reading);
    void processSampleQueue();

    int mNumBees;
    

    bool mFirstRead;
    unsigned long mInTime[NUM_GATES], mPrevInTime[NUM_GATES], mOutTime[NUM_GATES], mPrevOutTime[NUM_GATES];
    unsigned long mInDuration[NUM_GATES], mOutDuration[NUM_GATES], mNextActionTime, mPrevSampleTimestamp;
    unsigned char mReadState, mGate;
    uint32_t mBitMask, mPrevInReading, mPrevOutReading;
    bool mInState, mOutState, mPrevInState[NUM_GATES], mPrevOutState[NUM_GATES];

    // cache a bunch of hardware mappings to speed up ISRXS
    const PinDescription &mLoad, &mClk, &mData;
    PortGroup &mLoadReg, &mClkReg, &mDataReg;
    unsigned int mLoadMask, mClkMask, mDataMask;
    
    int dataPin;
};


#endif
