#ifndef StepperActuator_h
#define StepperActuator_h


#include <Actuator.h>
#include <pulsegen_consumer.h>

class Str;
class HiveConfig;
class RateProvider;

class StepperActuator : public Actuator {
 public:
    StepperActuator(const HiveConfig &config,
		    const RateProvider &rateProvider,
		    const char *name, unsigned long now,
		    int address, int port, bool isBackwards=false);
    // ports: 1==M1 & M2; 2==M3 & M4
    
    ~StepperActuator();

    int getLocation() const;
    int getTarget() const;

    bool loop(unsigned long now, Mutex *wifi);
    
    void processMsg(unsigned long now, const char *msg);

    bool isMyMsg(const char *msg) const;
    
    PulseGenConsumer *getPulseGenConsumer();

    void scheduleNextStep(unsigned long now) {setNextActionTime(now + mMsPerStep);}
    
 protected:
    virtual const char *className() const {return "StepperActuator";}

 private:
    friend class StepperActuatorPulseGenConsumer;
    
    bool isItTimeYetForSelfDrive(unsigned long now);
    void step();
    
    static StepperActuator **s_steppers;
    static bool s_pulseInitialized;
    static void PulseCallback();
    
    int mLoc, mTarget, mMsPerStep;
    bool mRunning, mIsBackwards;
    class Adafruit_StepperMotor *m;
    class Adafruit_MotorShield *AFMS;
};


inline int StepperActuator::getLocation() const
{
    return mLoc;
}

inline int StepperActuator::getTarget() const
{
    return mTarget;
}



#endif
