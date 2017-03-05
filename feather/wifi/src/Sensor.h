#ifndef sensor_h
#define sensor_h


class Str;
class StrBuf;
class Mutex;

class Sensor {
 public:
    Sensor(const char *sensorName,
	   const class RateProvider &rateProvider,
	   const class TimeProvider &timeProvider,
	   unsigned long now);
    ~Sensor();

    virtual bool isItTimeYet(unsigned long now);
    virtual bool loop(unsigned long now) = 0;

    const RateProvider &getRateProvider() const {return mRateProvider;}
    const TimeProvider &getTimeProvider() const {return mTimeProvider;}
    
    virtual const char *getName() const;
    
 protected:
    virtual const char *className() const = 0;
    
    StrBuf TAG(const char *memberfunc, const char *msg) const;
    
    unsigned long getNextSampleTime() const;
    void setNextSampleTime(unsigned long n);
    
 private:
    Sensor(const Sensor&);  // unimplemented
    const Sensor &operator=(const Sensor &); // unimplemented
    
    unsigned long mNextSampleTime;

    Str *mName;
    const class RateProvider &mRateProvider;
    const class TimeProvider &mTimeProvider;
};


inline
bool Sensor::isItTimeYet(unsigned long now)
{
    return now >= mNextSampleTime;
}


inline
unsigned long Sensor::getNextSampleTime() const
{
    return mNextSampleTime;
}

inline
void Sensor::setNextSampleTime(unsigned long n)
{
    mNextSampleTime = n;
}


#endif
