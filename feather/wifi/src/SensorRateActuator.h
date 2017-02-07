#ifndef sensorrate_h
#define sensorrate_h

#include <Actuator.h>
#include <RateProvider.h>


class Str;
class HiveConfig;

class SensorRateActuator : public Actuator, public RateProvider {
 public:
    SensorRateActuator(const HiveConfig &config, const char *sensorName, unsigned long now);
    ~SensorRateActuator() {}

    bool loop(unsigned long now, Mutex *wifi);

    bool isItTimeYet(unsigned long now) const {return now >= mNextActionTime;}
    void scheduleNextAction(unsigned long now);

    int secondsBetweenSamples() const {return mSeconds;}
    
 protected:
    virtual const char *className() const {return "SensorRateActuator";}

 private:
    int mSeconds;

    const HiveConfig &mConfig;
    class SensorRateGetter *mGetter;
};


#endif
