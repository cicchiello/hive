#ifndef BeeCounter_h
#define BeeCounter_h


#include <SensorBase.h>
#include <pulsegen_consumer.h>

class Str;


class BeeCounter : public SensorBase, private PulseGenConsumer {
 public:
    BeeCounter(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       const class TimeProvider &timeProvider,
	       unsigned long now,
	       int ploadPin, int clockPin, int dataPin);
    ~BeeCounter();

    bool sensorSample(Str *value);
    
    PulseGenConsumer *getPulseGenConsumer();
    
 private:
    const void *getSemaphore() const;
    
    const char *className() const {return "BeeCounter";}
    
    void pulse(unsigned long now);
    
    void readReg();

    static const int NUM_BYTES = 3;
    static const int NUM_GATES = NUM_BYTES*4;
    
    unsigned char mBytes[NUM_BYTES];
    unsigned char mOldBytes[NUM_BYTES];

    bool mFirstRead;
    unsigned char mPrevIn[NUM_GATES], mPrevOut[NUM_GATES];
    unsigned long mInTime[NUM_GATES], mPrevInTime[NUM_GATES], mOutTime[NUM_GATES], mPrevOutTime[NUM_GATES];
    unsigned long mInDuration[NUM_GATES], mOutDuration[NUM_GATES];
    int mNumBees;
    unsigned long mLastSampleTime;
    
    int ploadPin, clockEnablePin, clockPin, dataPin;
};


inline
const void *BeeCounter::getSemaphore() const
{
    return this;
}

#endif
