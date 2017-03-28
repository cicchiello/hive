#ifndef motorspeed_h
#define motorspeed_h

#include <Actuator.h>

class HiveConfig;

class MotorSpeedActuator : public Actuator {
 public:
    MotorSpeedActuator(HiveConfig *config, const char *sensorName, unsigned long now);
    ~MotorSpeedActuator() {}

    bool isMyMsg(const char *msg) const;
    void processMsg(unsigned long now, const char *msg);
    
    int stepsPerSecond() const;
    
    bool loop(unsigned long now);
    
 protected:
    virtual const char *className() const {return "MotorSpeedActuator";}

 private:
    int mStepsPerSecond;
    HiveConfig &mConfig;
};


#endif
