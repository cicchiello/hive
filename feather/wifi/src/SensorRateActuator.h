#ifndef sensorrate_h
#define sensorrate_h

#include <ActuatorBase.h>
#include <RateProvider.h>


class SensorRateActuator : public ActuatorBase, public RateProvider {
 public:
    SensorRateActuator(const HiveConfig &config, const char *sensorName, unsigned long now);
    ~SensorRateActuator() {}

    Getter *createGetter() const;
    
    void scheduleNextAction(unsigned long now);

    int secondsBetweenSamples() const {return mSeconds;}
    
    void processResult(ActuatorBase::Getter *getter);
    
 protected:
    virtual const char *className() const {return "SensorRateActuator";}

 private:
    int mSeconds;
};


#endif
