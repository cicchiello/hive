#ifndef sensor_h
#define sensor_h


class Str;
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
    
    Str TAG(const char *memberfunc, const char *msg) const;
    
    unsigned long mNextSampleTime;

 private:
    Sensor(const Sensor&);  // unimplemented
    const Sensor &operator=(const Sensor &); // unimplemented
    
    Str *mName;
    const class RateProvider &mRateProvider;
    const class TimeProvider &mTimeProvider;
};


#endif
