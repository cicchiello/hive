#ifndef LimitStepperActuator_h
#define LimitStepperActuator_h


#include <StepperActuator.h>

class LimitStepperActuator : public StepperActuator {
 public:
    LimitStepperActuator(const HiveConfig &config,
			 const RateProvider &rateProvider,
			 const MotorSpeedActuator &motorSpeedProvider,
			 const char *name, unsigned long now,
			 int positiveLimitPin, int negativeLimitPin,
			 int address, int port, bool isBackwards=false);
    // ports: 1==M1 & M2; 2==M3 & M4

    enum StepperState {
      Stopped,
      MovingPositive,
      MovingNegative,
      AtPositiveLimit,
      AtNegativeLimit
    };
    StepperState getState() const {return mState;}
    
 protected:
    virtual const char *className() const {return "LimitStepperActuator";}

    virtual void start();
    virtual void step();
    
 private:
    bool isAtPosLimit() const;
    bool isAtNegLimit() const;
    
    int mPositivePin, mNegativePin;
    StepperState mState;
};



#endif
