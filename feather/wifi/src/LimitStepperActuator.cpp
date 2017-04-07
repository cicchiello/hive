#include <LimitStepperActuator.h>

#include <Arduino.h>


//#define HEADLESS
#define NDEBUG

#include <Trace.h>


static inline bool atLimit(int pin)
{
    return digitalRead(pin) ? false : true;
}


LimitStepperActuator::LimitStepperActuator(const HiveConfig &config,
					   const RateProvider &rateProvider,
					   const MotorSpeedActuator &motorSpeedProvider,
					   const char *name,
					   unsigned long now,
					   int positiveLimitPin, int negativeLimitPin,
					   int address, int port,
					   bool isBackwards)
  : StepperActuator(config, rateProvider, motorSpeedProvider, name, now, address, port, isBackwards),
    mPositivePin(positiveLimitPin), mNegativePin(negativeLimitPin), mState(LimitStepperActuator::Stopped)
{
    TF("LimitStepperActuator::LimitStepperActuator");

    pinMode(positiveLimitPin, INPUT_PULLUP);
    pinMode(negativeLimitPin, INPUT_PULLUP);
}


bool LimitStepperActuator::isAtPosLimit() const
{
    return atLimit(mPositivePin);
}


bool LimitStepperActuator::isAtNegLimit() const
{
    return atLimit(mNegativePin);
}


void LimitStepperActuator::step()
{
    TF("LimitStepperActuator::step");

    if (getLocation() != getTarget()) {
        bool atPosLimit = atLimit(mPositivePin);
	bool atNegLimit = atLimit(mNegativePin);
	if ((getTarget() > getLocation()) && atPosLimit) {
	    PH("Limiting postive motion due to limit switch closure");
	    stop();
	    mState = AtPositiveLimit;
	} else if ((getTarget() < getLocation()) && atNegLimit) {
	    PH("Limiting negative motion due to limit switch closure");
	    stop();
	    mState = AtNegativeLimit;
	} else {
	    StepperActuator::step();
	    if (getLocation() == getTarget())
	        mState = Stopped;
	}
    }
}


void LimitStepperActuator::start()
{
    TF("LimitStepperActuator::start");
    
    bool atPosLimit = atLimit(mPositivePin);
    bool atNegLimit = atLimit(mNegativePin);
    if (getTarget() > getLocation()) {
        PH("request received to move in positive direction");
	if (!atPosLimit) {
	    StepperActuator::start();
	    mState = MovingPositive;
	} else {
	    PH("WARNING: Request is ignored since the positive limit switch is closed");
	}
    } else if (getTarget() < getLocation()) {
        PH("request received to move in negative direction");
	if (!atNegLimit) {
	    StepperActuator::start();
	    mState = MovingNegative;
	} else {
	    PH("WARNING: Request is ignored since the negative limit switch is closed");
	}
    } else {
        PH("WARNING: start request ignored since getTarget()==getLocation()");
    }
}


