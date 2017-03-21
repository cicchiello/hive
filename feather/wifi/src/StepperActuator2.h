#ifndef StepperActuator2_h
#define StepperActuator2_h


#include <Actuator.h>
#include <pulsegen_consumer.h>

class Str;
class HiveConfig;
class RateProvider;

class StepperActuator2 : public Actuator {
 public:
    StepperActuator2(const HiveConfig &config,
		     const RateProvider &rateProvider,
		     const char *name, unsigned long now,
		     int address, int port, bool isBackwards=false);
    // ports: 1==M1 & M2; 2==M3 & M4
    
    ~StepperActuator2();

    int getLocation() const;
    int getTarget() const;

    bool loop(unsigned long now);
    
    void processMsg(unsigned long now, const char *msg);

    bool isMyMsg(const char *msg) const;
    
 protected:
    virtual const char *className() const {return "StepperActuator2";}

 private:
    friend class StepperActuator2PulseGenConsumer;

    int mTarget, mPort, mI2Caddr, mStepperIndex;
    bool mRunning, mIsBackwards;
};


inline int StepperActuator2::getLocation() const
{
    return 0;
}

inline int StepperActuator2::getTarget() const
{
    return mTarget;
}



#endif
