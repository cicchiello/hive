#ifndef StepperActuator_h
#define StepperActuator_h


#include <Actuator.h>

class Str;


class StepperActuator : public Actuator {
 public:
    StepperActuator(const char *name, int port, unsigned long now, int stepsPerCall);
    // ports: 1==M1 & M2; 2==M3 & M4
    
    ~StepperActuator();

    int getLocation() const;
    const char *getName() const;
    
    void act();

    bool isItTimeYet(unsigned long now);
    void scheduleNextAction(unsigned long now);
    
 private:
    Str *mPrev, *mName;
    int mLoc, mStepsPerCall, mMsPerStep;
    unsigned long mLastSampleTime;
    class Adafruit_StepperMotor *m;
    class Adafruit_MotorShield *AFMS;
};


#endif
