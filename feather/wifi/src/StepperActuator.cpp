#include <StepperActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>


#include <hiveconfig.h>

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <utility/Adafruit_MS_PWMServoDriver.h>

#include <hive_platform.h>

#include <MotorSpeedActuator.h>

#include <str.h>
#include <strutils.h>

#define REVSTEPS 200.0

extern "C" {void GlobalYield();};


class StepperActuatorPulseGenConsumer : public PulseGenConsumer {
private:
  StepperActuatorPulseGenConsumer();

  StepperActuator **mSteppers;

public:
  static StepperActuatorPulseGenConsumer *nonConstSingleton();

  void addStepper(StepperActuator *stepper);
  void removeStepper(StepperActuator *stepper);

  void pulse(unsigned long now);
};


void StepperActuatorPulseGenConsumer::addStepper(StepperActuator *stepper)
{
    TF("StepperActuatorPulseGenConsumer::addStepper");

    // if there are no steppers currently on the ISR handler list, then let's start the pulse generator
    if (mSteppers[0] == NULL) {
        HivePlatform::nonConstSingleton()->registerPulseGenConsumer_11K(StepperActuatorPulseGenConsumer::nonConstSingleton());
    }
    
    // put given StepperActuator on the ISR handler list
    // (consider that it might already be there)
    bool found = false;
    int i = 0;
    while (!found && mSteppers[i]) {
        found = mSteppers[i++] == stepper;
    }

    assert(!found, "Stepper already on the list!?!?");

    TRACE2("adding stepper to list at entry: ", i);
    mSteppers[i] = stepper;
}


void StepperActuatorPulseGenConsumer::removeStepper(StepperActuator *stepper)
{
    // remove given StepperActuator from the ISR handler list
    TF("StepperActuatorPulseGenConsumer::removeStepper");
    
    int i = 0;
    while ((mSteppers[i] != stepper) && mSteppers[i]) 
        i++;
    
    assert(mSteppers[i], "Stepper wasn't found on the ISR handler list!?!?");

    TRACE("Removing stepper from ISR handler list");
    int j = 0;
    while (mSteppers[j])
        j++;
    j--; // went one too far
    if (i == j) {
        mSteppers[i] = NULL;
    } else {
        mSteppers[i] = mSteppers[j];
	mSteppers[j] = NULL;
    }

    // if the ISR Handler list is empty, stop the pulse generator
    if (mSteppers[0] == NULL) {
        HivePlatform::nonConstSingleton()->unregisterPulseGenConsumer_11K(StepperActuatorPulseGenConsumer::nonConstSingleton());
    }
}


StepperActuatorPulseGenConsumer::StepperActuatorPulseGenConsumer()
{
    mSteppers = new StepperActuator*[10];
    for (int i = 0; i < 10; i++)
        mSteppers[i] = NULL;
}

/* STATIC */
StepperActuatorPulseGenConsumer *StepperActuatorPulseGenConsumer::nonConstSingleton()
{
    static StepperActuatorPulseGenConsumer sSingleton;
    return &sSingleton;
}


void StepperActuatorPulseGenConsumer::pulse(unsigned long now)
{
    TF("StepperActuatorPulseGenConsumer::pulse");
    
    //to see the pulse on the scope, enable "5" as an output and uncomment:
    //const PinDescription &p = g_APinDescription[5];
    //PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
    
    bool didSomething = true;
    while (didSomething) {
        didSomething = false;
	int i = 0;
	while (!didSomething && mSteppers[i]) {
	    if (mSteppers[i]->isItTimeYetForSelfDrive(now)) {

	        mSteppers[i]->scheduleNextStep(now); // must be done before step!
		// (step may remove the stepper from this list we're iterating!)
		
	        mSteppers[i]->step();
		
		now = millis();
		didSomething = true;
	    }
	    i++;
	}
    }
}





PulseGenConsumer *StepperActuator::getPulseGenConsumer()
{
    return StepperActuatorPulseGenConsumer::nonConstSingleton();
}

StepperActuator::StepperActuator(const HiveConfig &config,
				 const RateProvider &rateProvider,
				 const MotorSpeedActuator &motorSpeedProvider,
				 const char *name,
				 unsigned long now,
				 int address, int port,
				 bool isBackwards)
  : Actuator(name, now), mLoc(0), mTarget(0), mMotorSpeedProvider(motorSpeedProvider),
    mRunning(false), mIsBackwards(isBackwards)
{
    TF("StepperActuator::StepperActuator");
    
    AFMS = new Adafruit_MotorShield(address);
    AFMS->begin();
    m = AFMS->getStepper((int)REVSTEPS, port); // 200==steps per revolution; ports 1==M1 & M2, 2==M3 & M4
}


StepperActuator::~StepperActuator()
{
    TF("StepperActuator::~StepperActuator");

    if (m != NULL)
        m->release();

    delete AFMS;
    delete m;
}


void StepperActuator::step()
{
    TF("StepperActuator::step");

    if (mLoc != mTarget) {
        int dir = mLoc < mTarget ?
			 (mIsBackwards ? BACKWARD : FORWARD) :
	                 (mIsBackwards ? FORWARD : BACKWARD);
	mLoc += mLoc < mTarget ? 1 : -1;

	//D("StepperActuator::act stepping: "); D(getName()); D(" "); DL(dir);
	m->onestep(dir, DOUBLE);

	//to see a pulse on the scope, enable "5" as an output and uncomment:
	//const PinDescription &p = g_APinDescription[5];
	//PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
	
	if (mLoc == mTarget) {
	    PH("Stopped");
	    
	    m->release();
	    mRunning = false;

	    // remove this StepperActuator from the ISR handler list
	    StepperActuatorPulseGenConsumer::nonConstSingleton()->removeStepper(this);

	    mLoc = mTarget = 0;
	    TRACE("Released the stepper motor");
	}
    }
}


bool StepperActuator::isItTimeYetForSelfDrive(unsigned long now)
{
    TF("StepperActuator::isItTimeYetForSelfDrive");
    unsigned long when = getNextActionTime();
    if ((now > when) && (mLoc != mTarget)) {
        TRACE4("StepperActuator::isItTimeYetForSelfDrive; missed an appointed visit by ",
	       now - when, " ms; now == ", now);
    }
    
    return now >= when;
}


bool StepperActuator::loop(unsigned long now)
{
    return mLoc != mTarget;
}


void StepperActuator::processMsg(unsigned long now, const char *msg)
{
    static const char *InvalidMsg = "invalid msg; did isMyMsg return true?";
    
    TF("StepperActuator::processMsg");

    const char *name = getName();
    int namelen = strlen(name);
    assert2(strncmp(msg, name, namelen) == 0, InvalidMsg, msg);
    
    msg += namelen;
    const char *token = "|";
    int tokenlen = 1;
    assert2(strncmp(msg, token, tokenlen) == 0, InvalidMsg, msg);
    msg += tokenlen;
    assert2(StringUtils::isNumber(msg), InvalidMsg, msg);

    int newTarget = atoi(msg);
    if (newTarget != mTarget) {
        TRACE2("mTarget was: ", mTarget);
	TRACE2("new target is: ", newTarget);

	GlobalYield();
	
        mTarget = newTarget;
        if (mTarget != mLoc) {
	    PH3("Running for ", (mTarget-mLoc), " steps");

	    int rpm = 3*mMotorSpeedProvider.stepsPerSecond()/10;
	    m->setSpeed(rpm);

	    double dmsPerStep = 1000.0/mMotorSpeedProvider.stepsPerSecond();
	    mMsPerStep = (int) dmsPerStep;
	    PH2("setting rpm to ", rpm);
	    PH3("scheduling steps at the rate of once per ", mMsPerStep, " ms");
    
	    // put this StepperActuator on the ISR handler list
	    // (consider that it might already be there)
	    StepperActuatorPulseGenConsumer::nonConstSingleton()->addStepper(this);

	    scheduleNextStep(millis());
	    
	    mRunning = true;
	}
    }
}


bool StepperActuator::isMyMsg(const char *msg) const
{
    TF("StepperActuator::isMyMsg");
    const char *name = getName();
    TRACE2("isMyMsg for motor: ", name);
    TRACE2("Considering msg: ", msg);
    
    if (strncmp(msg, name, strlen(name)) == 0) {
        msg += strlen(name);
	const char *token = "|";
	bool r = (strncmp(msg, token, strlen(token)) == 0) && StringUtils::isNumber(msg+strlen(token));
	if (r) {
	    TRACE("it's mine!");
PH2("it's mine!  now: ", millis());
	}
	return r;
    } else {
        return false;
    }
}

