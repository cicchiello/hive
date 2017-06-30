#ifndef SensorBase_h
#define SensorBase_h


#include <Sensor.h>

class Str;
class Mutex;
class HiveConfig;
class HttpJSONConsumer;


class SensorBase : public Sensor {
 public:
    SensorBase(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       unsigned long now, Mutex *wifiMutex);
    ~SensorBase();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);

    virtual bool sensorSample(Str *value) = 0;

    Mutex *getWifiMutex() const {return mWifiMutex;}

    static const Str UNDEF; // used to indicate that SensorBase should not POST
    
 protected:
    const HiveConfig &getConfig() const {return mConfig;}

    void enableValueCache(bool v) {mValueCacheEnabled = v;}
    
    void setSample(const Str &value);
    
    unsigned long getNextPostTime() const;
    void setNextPostTime(unsigned long n);
    
    bool postImplementation(unsigned long now, Mutex *wifi, bool *success);
    
    virtual bool processResult(const HttpJSONConsumer &consumer, unsigned long *callMeBackIn_ms,
			       bool *keepMutex, bool *success);
    
 private:
    unsigned long mNextPostTime;

    Str *mValueStr, *mPrevValueStr;
    const HiveConfig &mConfig;
    class HttpJSONPost *mPoster;
    Mutex *mWifiMutex;
    bool mDidFirstPost, mValueCacheEnabled;
};

inline
bool SensorBase::isItTimeYet(unsigned long now)
{
    return (now >= mNextPostTime) || Sensor::isItTimeYet(now);
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
