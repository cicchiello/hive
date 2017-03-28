#ifndef BeeCounter_h
#define BeeCounter_h


#include <SensorBase.h>

class Str;


class BeeCounter : public SensorBase {
 public:
    BeeCounter(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       unsigned long now,
	       int ploadPin, int clockPin, int dataPin,
	       Mutex *wifiMutex);
    ~BeeCounter();

    bool sensorSample(Str *value);
    
    bool sample(unsigned long now);

 private:
    const char *className() const {return "BeeCounter";}
    
    void readReg();

    static const int NUM_BYTES = 3;
    static const int NUM_GATES = NUM_BYTES*4;
    
    unsigned char mBytes[NUM_BYTES];
    unsigned char mOldBytes[NUM_BYTES];

    bool mFirstRead, mMidnightInitialized;
    unsigned char mPrevIn[NUM_GATES], mPrevOut[NUM_GATES];
    unsigned long mInTime[NUM_GATES], mPrevInTime[NUM_GATES], mOutTime[NUM_GATES], mPrevOutTime[NUM_GATES];
    unsigned long mInDuration[NUM_GATES], mOutDuration[NUM_GATES];
    int mNumBeesIn, mNumBeesOut;
    unsigned long mLastSampleTime, mNextMidnight;
    
    int ploadPin, clockEnablePin, clockPin, dataPin;
};


#endif
