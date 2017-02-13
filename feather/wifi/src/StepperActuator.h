#ifndef StepperActuator_h
#define StepperActuator_h


#include <ActuatorBase.h>
#include <pulsegen_consumer.h>

class Str;

class StepperActuator : public ActuatorBase {
 public:
    StepperActuator(const HiveConfig &config,
		    const RateProvider &rateProvider,
		    const char *name, unsigned long now,
		    int address, int port, bool isBackwards=false);
    // ports: 1==M1 & M2; 2==M3 & M4
    
    ~StepperActuator();

    void setNextActionTime(unsigned long now);
    
    int getLocation() const;
    int getTarget() const;

    void processResult(ActuatorBase::Getter *getter);

    PulseGenConsumer *getPulseGenConsumer();
    
 protected:
    virtual const char *className() const {return "StepperActuator";}

 private:
    friend class StepperActuatorPulseGenConsumer;
    
    Getter *createGetter() const;

    bool isItTimeYetForSelfDrive(unsigned long now);
    void step();
    
    static StepperActuator **s_steppers;
    static bool s_pulseInitialized;
    static void PulseCallback();
    
    Str *mPrev, *mName;
    int mLoc, mTarget, mMsPerStep;
    unsigned long mLastSampleTime;
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

inline
void StepperActuator::setNextActionTime(unsigned long now)
{
    setNextActionTime(now + mMsPerStep);
}



#endif
