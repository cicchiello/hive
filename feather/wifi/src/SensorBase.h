#ifndef SensorBase_h
#define SensorBase_h


#include <Sensor.h>

class Str;
class Mutex;
class HiveConfig;
class HttpCouchConsumer;


class SensorBase : public Sensor {
 public:
    SensorBase(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       const class TimeProvider &timeProvider,
	       unsigned long now);
    ~SensorBase();

    bool loop(unsigned long now, Mutex *wifi);

    virtual bool sensorSample(Str *value) = 0;

 protected:
    const HiveConfig &getConfig() const {return mConfig;}
    
    unsigned long getNextSampleTime() const;
    void setNextSampleTime(unsigned long n);
    
    unsigned long getNextPostTime() const;
    void setNextPostTime(unsigned long n);
    
    bool postImplementation(unsigned long now, Mutex *wifi);
    
    virtual bool processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms);
    
 private:
    SensorBase(const SensorBase &); // intentionally unimplemented
    const SensorBase &operator=(const SensorBase &o); // intentionallly unimplemented
    
    unsigned long mNextSampleTime, mNextPostTime;

    Str *mValueStr;
    const HiveConfig &mConfig;
    class HttpCouchPost *mPoster;
};

inline
unsigned long SensorBase::getNextSampleTime() const
{
    return mNextSampleTime;
}

inline
void SensorBase::setNextSampleTime(unsigned long n)
{
    mNextSampleTime = n;
}

inline
unsigned long SensorBase::getNextPostTime() const
{
    return mNextPostTime;
}

inline
void SensorBase::setNextPostTime(unsigned long n)
{
    mNextPostTime = n;
}

#endif
