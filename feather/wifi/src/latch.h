#ifndef latch_h
#define latch_h


#include <SensorBase.h>
#include <pulsegen_consumer.h>

class Str;
class TempSensor;
class ServoConfig;


class Latch : public SensorBase, private PulseGenConsumer {
 public:
    Latch(const HiveConfig &config,
	  const char *name,
	  const class RateProvider &rateProvider,
	  const class TimeProvider &timeProvider,
	  unsigned long now,
	  int servoPin, const TempSensor &tempSensor, const ServoConfig &servoConfig);
    ~Latch();

    bool sensorSample(Str *value);
    
    PulseGenConsumer *getPulseGenConsumer();
    
 private:
    const char *className() const {return "Latch";}
    
    void pulse(unsigned long now);

    void clockwise(unsigned long now);
    void counterclockwise(unsigned long now);

    class LocalServoConfig *mCurrentConfig;
    const TempSensor &mTempSensor;
    const ServoConfig &mServoConfigProvider;
    double mTemp;
    
    int mServoPin, mTicksOn, mTicksOff, mTicks;
    unsigned long mStartedMoving, mLastTempTime;
    bool mIsHigh, mIsMoving, mIsAtClockwiseLimit, mFirstPulse;
    
};


inline
PulseGenConsumer *Latch::getPulseGenConsumer()
{
    return this;
}



#endif
