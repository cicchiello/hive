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

#include <str.h>

#define REVSTEPS 200
#define RPM 60

static bool s_runningNotification = false;

StepperActuator::StepperActuator(const char *name, int port, unsigned long now)
  : Actuator(now), mName(new Str(name)), mLoc(0), mTarget(0)
{
    AFMS = new Adafruit_MotorShield();
    AFMS->begin();
    mLastSampleTime = now;;
    m = AFMS->getStepper(REVSTEPS, port); // 200==steps per revolution; ports 1==M1 & M2, 2==M3 & M4
    m->setSpeed(RPM);

    int usPerStep = 60000000 / ((uint32_t)REVSTEPS * (uint32_t)RPM);
    mMsPerStep = usPerStep/1000;
    D("Scheduling steps at the rate of once per ");
    D(mMsPerStep);
    DL(" ms");
}


StepperActuator::~StepperActuator()
{
    DL("StepperActuator::~StepperActuator called");
    
    delete mName;
    delete AFMS;
    delete m;
}


const char *StepperActuator::getName() const
{
    return mName->c_str();
}


void StepperActuator::act()
{
    //DL("StepperActuator::sensorSample called");

    if (mLoc != mTarget) {
        if (s_runningNotification) {
	    PL("Running...");
	    s_runningNotification = false;
	}
	
        int dir = mLoc < mTarget ? FORWARD : BACKWARD;
	mLoc += mLoc < mTarget ? 1 : -1;

	m->onestep(dir, DOUBLE);

	if (mLoc == mTarget) {
	    m->release();
	    PL("Released the stepper motor");
	}
    }
}


bool StepperActuator::isItTimeYet(unsigned long now)
{
#ifdef foo  
    if ((now > mNextActionTime+3) && (mLoc != mTarget)) {
        P("StepperActuator::isItTimeYet; missed an appointed visit by ");
	P(now - mNextActionTime);
	P(" ms; now == ");
	PL(now);
    }
#endif
    
    return now >= mNextActionTime;
}


void StepperActuator::scheduleNextAction(unsigned long now)
{
    mNextActionTime = now + mMsPerStep;
}


bool StepperActuator::isMyCommand(const char *msg) const
{
    D("StepperActuator::isMyCommand; testing msg: "); DL(msg);
    const char *prefix = "action|";
    if (strncmp(msg, prefix, strlen(prefix)) == 0) {
        msg = msg + strlen(prefix);
	Str targetName = *mName;
	targetName.append("-target");
	if (strncmp(msg, targetName.c_str(), targetName.len()) == 0) {
	    DL("StepperActuator::isMyCommand; found one of mine!");
	    return true;
	}
    }
    return false;
}


static char *consumeToEOL(const char *rsp)
{
    char *c = (char*) rsp;
    while (*c && ((*c >= '0') && (*c <= '9')))
        c++;
    if (*c == '\n' || *c == '\l')
        c++;
    if (*c == '\\' && *(c+1) == 'n')
        c += 2;
    return c;
}


static bool s_first = true;
char *StepperActuator::processCommand(const char *msg)
{
    const char *prefix = "action|";
    Str targetName = *mName;
    targetName.append("-target");
    Str command(msg + strlen(prefix) + targetName.len() + 1);
    D("StepperActuator::processCommand; parsing: "); DL(command.c_str());
    int newTarget = atoi(command.c_str());
    D("StepperActuator::processCommand; target set to "); DL(newTarget);

    if (newTarget != mTarget) {
        mTarget = newTarget;
        if (mTarget != mLoc)
	    s_runningNotification = true;
    }

    return consumeToEOL(command.c_str());
}


