#ifndef StepperActuator_h
#define StepperActuator_h


#include <Actuator.h>
#include <pulsegen_consumer.h>

class Str;


class StepperActuator : public Actuator {
 public:
    StepperActuator(const char *name, int address, int port, unsigned long now,
		    bool isBackwards=false);
    // ports: 1==M1 & M2; 2==M3 & M4
    
    ~StepperActuator();

    int getLocation() const;
    int getTarget() const;
    
    virtual void act(class Adafruit_BluefruitLE_SPI &ble);

    bool isItTimeYet(unsigned long now) const;
    void scheduleNextAction(unsigned long now);
    
    bool isMyCommand(const Str &response) const;
    void processCommand(Str *response);

    PulseGenConsumer *getPulseGenConsumer();
    
 protected:
    virtual const char *className() const {return "StepperActuator";}

 private:
    friend class StepperActuatorPulseGenConsumer;
    
    void setNextActionTime(unsigned long now);
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



#endif
