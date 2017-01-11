#ifndef StepperMonitor_h
#define StepperMonitor_h


#include <Sensor.h>

class Str;


class StepperMonitor : public Sensor {
 public:
    StepperMonitor(const class StepperActuator &actuator, unsigned long now);
    ~StepperMonitor();

    bool sensorSample(Str *value);
    
    void scheduleNextSample(unsigned long now);
    
 private:
    const class StepperActuator &mActuator;
    Str *mPrev;
};


#endif
