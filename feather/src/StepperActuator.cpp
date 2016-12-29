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

StepperActuator::StepperActuator(const char *name, int port, unsigned long now, int stepsPerCall)
: Actuator(now), mName(new Str(name)), mLoc(0), mStepsPerCall(stepsPerCall)
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


int StepperActuator::getLocation() const
{
    return mLoc;
}


const char *StepperActuator::getName() const
{
    return mName->c_str();
}


void StepperActuator::act()
{
    DL("StepperActuator::sensorSample called");

    if (mStepsPerCall > 0) {
        m->onestep(FORWARD, DOUBLE);
	mLoc++;
    } else {
        m->onestep(BACKWARD, DOUBLE);
	mLoc--;
    }
}


bool StepperActuator::isItTimeYet(unsigned long now)
{
    if (now > mNextActionTime+3) {
        P("StepperActuator::isItTimeYet; missed an appointed visit by ");
	P(now - mNextActionTime);
	P(" ms; now == ");
	PL(now);
    }
    
    return now >= mNextActionTime;
}


void StepperActuator::scheduleNextAction(unsigned long now)
{
    mNextActionTime = now + mMsPerStep;
}


