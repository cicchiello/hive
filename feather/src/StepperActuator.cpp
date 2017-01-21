#include <StepperActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif


#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <utility/Adafruit_MS_PWMServoDriver.h>

#include <hive_platform.h>

#include <str.h>
#include <strutils.h>

#define REVSTEPS 200
#define RPM 60


#define ERROR(msg)     HivePlatform::singleton()->error(msg)
#define TRACE(msg)     HivePlatform::singleton()->trace(msg)


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
    // put given StepperActuator on the ISR handler list
    // (consider that it might already be there)
    bool found = false;
    int i = 0;
    while (!found && mSteppers[i]) {
        found = mSteppers[i++] == stepper;
    }
    if (!found) {
        D(StringUtils::TAG("StepperActuatorPulseGenConsumer::addStepper",
			   "adding stepper to list at entry ").c_str());
	DL(i);
	mSteppers[i] = stepper;
    } else {
        ERROR("StepperActuatorPulseGenConsumer::addStepper; "
	      "Stepper already on the list!?!?");
    }
}


void StepperActuatorPulseGenConsumer::removeStepper(StepperActuator *stepper)
{
    // remove given StepperActuator from the ISR handler list
    int i = 0;
    while ((mSteppers[i] != stepper) && mSteppers[i]) 
        i++;
    if (mSteppers[i]) {
        DL(StringUtils::TAG("StepperActuatorPulseGenConsumer::removeStepper",
			    "Removing stepper from ISR handler list").c_str());
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
    } else {
        ERROR("StepperActuatorPulseGenConsumer::removeStepper; "
	      "Stepper wasn't found on the ISR handler list!?!?");
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
    //DL("StepperPulseCallback; ");

    //to see the pulse on the scope, enable "5" as an output and uncomment:
    //const PinDescription &p = g_APinDescription[5];
    //PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
    
    bool didSomething = true;
    while (didSomething) {
        didSomething = false;
	int i = 0;
	while (!didSomething && mSteppers[i]) {
	    if (mSteppers[i]->isItTimeYetForSelfDrive(now)) {

	        mSteppers[i]->setNextActionTime(now); // must be done before step!
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

StepperActuator::StepperActuator(const char *name, int address, int port, unsigned long now,
				 bool isBackwards)
  : Actuator(name, now), mName(new Str(name)), mLoc(0), mTarget(0),
    mRunning(false), mIsBackwards(isBackwards)
{
    TRACE("StepperActuator CTOR; entry");
    AFMS = new Adafruit_MotorShield(address);
    AFMS->begin();
    mLastSampleTime = now;
    m = AFMS->getStepper(REVSTEPS, port); // 200==steps per revolution; ports 1==M1 & M2, 2==M3 & M4
    m->setSpeed(RPM);

    int usPerStep = 60000000 / ((uint32_t)REVSTEPS * (uint32_t)RPM);
    D(TAG("StepperActuator", "scheduling steps at the rate of once per ").c_str());
    D(usPerStep);
    DL(" us");
    mMsPerStep = usPerStep/1000;
    
    TRACE("StepperActuator CTOR; exit");
}


StepperActuator::~StepperActuator()
{
    DL("StepperActuator::~StepperActuator called");

    if (m != NULL)
        m->release();

    delete mName;
    delete AFMS;
    delete m;
}


void StepperActuator::act(class Adafruit_BluefruitLE_SPI &ble)
{
  // no-op
}


void StepperActuator::step()
{
static bool sCalledAtLeastOnce = false;
if (!sCalledAtLeastOnce) {
  DL(TAG("step", "called at least once").c_str());
  sCalledAtLeastOnce = true;
}

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
	    DL(TAG("processCommand", "Stopped.").c_str());
	    m->release();
	    mRunning = false;

	    // remove this StepperActuator from the ISR handler list
	    StepperActuatorPulseGenConsumer::nonConstSingleton()->removeStepper(this);

	    DL(TAG("processCommand", "Released the stepper motor").c_str());
	}
    }
}


bool StepperActuator::isItTimeYet(unsigned long now) const
{
    return true;
}


bool StepperActuator::isItTimeYetForSelfDrive(unsigned long now)
{
    if ((now > mNextActionTime) && (mLoc != mTarget)) {
        D("StepperActuator::isItTimeYet; missed an appointed visit by ");
	D(now - mNextActionTime);
	D(" ms; now == ");
	DL(now);
    }
    
    return now >= mNextActionTime;
}


void StepperActuator::scheduleNextAction(unsigned long now)
{
  // no-op
}


void StepperActuator::setNextActionTime(unsigned long now)
{
    mNextActionTime = now + mMsPerStep;
}


bool StepperActuator::isMyCommand(const Str &msg) const
{
    //DL("StepperActuator::isMyResponse");
    const char *token = "action|";
    const char *cmsg = msg.c_str();
    
    if (strncmp(cmsg, token, strlen(token)) == 0) {
        cmsg += strlen(token);
	const char *name = getName();
	if (strncmp(cmsg, name, strlen(name)) == 0) {
	    cmsg += strlen(name);
	    token = "-target|";
	    bool r = (strncmp(cmsg, token, strlen(token)) == 0);
	    if (r) {
	        DL(TAG("isMyCommand", "claiming a response").c_str());
	    }
	    return r;
	}
    } 
    return false;
}


void StepperActuator::processCommand(Str *msg)
{
    const char *token0 = "action|";
    const char *token1 = getName();
    const char *token2 = "-target|";
    const char *cmsg = msg->c_str() + strlen(token0) + strlen(token1) + strlen(token2);

    D(TAG("processCommand", "parsing: ").c_str());
    DL(cmsg);
    int newTarget = atoi(cmsg);
    if (newTarget != mTarget) {
        D(TAG("processCommand", "mTarget was ").c_str());
        DL(mTarget);
        D(TAG("processCommand", "new target is ").c_str());
	DL(newTarget);
        mTarget = newTarget;
        if (mTarget != mLoc) {
	    DL(TAG("processCommand", "Running...").c_str());

	    // put this StepperActuator on the ISR handler list
	    // (consider that it might already be there)
	    StepperActuatorPulseGenConsumer::nonConstSingleton()->addStepper(this);

	    setNextActionTime(millis());
	    
	    mRunning = true;
	}
    }

    *msg = cmsg;
    StringUtils::consumeToEOL(msg);
}
