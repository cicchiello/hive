#ifndef SensorBase_h
#define SensorBase_h


#include <Sensor.h>

class Str;
class Mutex;
class HiveConfig;


class SensorBase : public Sensor {
 public:
    SensorBase(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       const class TimeProvider &timeProvider,
	       unsigned long now);
    ~SensorBase();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now, Mutex *wifi);

    virtual bool sensorSample(Str *value) = 0;

 protected:
    unsigned long getNextSampleTime() const;
    void setNextSampleTime(unsigned long n);
    
    unsigned long getNextPostTime() const;
    void setNextPostTime(unsigned long n);
    
    // helper function
    static void setNextTime(unsigned long now, unsigned long *t);

    void postImplementation(unsigned long now, Mutex *wifi);
    
 private:
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
