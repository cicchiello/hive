#ifndef sensorrate_h
#define sensorrate_h

#include <Actuator.h>
#include <RateProvider.h>

class HiveConfig;

class SensorRateActuator : public Actuator, public RateProvider {
 public:
    SensorRateActuator(HiveConfig *config, const char *sensorName, unsigned long now);
    ~SensorRateActuator() {}

    bool isMyMsg(const char *msg) const;
    void processMsg(unsigned long now, const char *msg);
    
    int secondsBetweenSamples() const;
    
    bool loop(unsigned long now);
    
 protected:
    virtual const char *className() const {return "SensorRateActuator";}

 private:
    int mSeconds;
    HiveConfig &mConfig;
};


#endif
