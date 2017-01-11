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

#include <platformutils.h>

#include <Parse.h>
#include <str.h>

#define REVSTEPS 200
#define RPM 60


/* STATIC */
StepperActuator **StepperActuator::s_steppers = NULL;
bool StepperActuator::s_pulseInitialized = false;


/* STATIC */
void StepperActuator::PulseCallback()
{
    //PL("StepperPulseCallback; ");

    //to see the pulse on the scope, enable "5" as an output and uncomment:
    //const PinDescription &p = g_APinDescription[5];
    //PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
    
    unsigned long now = millis();
    bool didSomething = true;
    while (didSomething) {
        didSomething = false;
	int i = 0;
	while (!didSomething && s_steppers[i]) {
	    if (s_steppers[i]->isItTimeYetForSelfDrive(now)) {
	      
	        s_steppers[i]->scheduleNextAction(now); // must be done before act!
		// (act may remove the stepper from this list we're iterating!)
		
	        s_steppers[i]->act();
		
		now = millis();
		didSomething = true;
	    }
	    i++;
	}
    }
}


StepperActuator::StepperActuator(const char *name, int address, int port, unsigned long now,
				 bool isBackwards)
  : Actuator(now), mName(new Str(name)), mLoc(0), mTarget(0),
    mRunning(false), mIsBackwards(isBackwards)
{
    WDT_TRACE("StepperActuator CTOR; entry");
    AFMS = new Adafruit_MotorShield(address);
    AFMS->begin();
    mLastSampleTime = now;
    m = AFMS->getStepper(REVSTEPS, port); // 200==steps per revolution; ports 1==M1 & M2, 2==M3 & M4
    m->setSpeed(RPM);

    int usPerStep = 60000000 / ((uint32_t)REVSTEPS * (uint32_t)RPM);
    D("Scheduling steps at the rate of once per ");
    D(usPerStep);
    DL(" us");
    mMsPerStep = usPerStep/1000;
    D("Scheduling steps at the rate of once per ");
    D(mMsPerStep);
    DL(" ms");

    if (!s_pulseInitialized) {
        WDT_TRACE("StepperActuator CTOR; initializing pulse generator");
        PlatformUtils::nonConstSingleton().initPulseGenerator(0, SAMPLES_PER_SECOND);
	PlatformUtils::nonConstSingleton().startPulseGenerator(0, StepperActuator::PulseCallback);

	s_steppers = new StepperActuator*[10];
	for (int i = 0; i < 10; i++)
	    s_steppers[i] = NULL;

	s_pulseInitialized = true;
    }
    WDT_TRACE("StepperActuator CTOR; exit");
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


const char *StepperActuator::getName() const
{
    return mName->c_str();
}


void StepperActuator::act()
{
    //DL("StepperActuator::act called");

    if (mLoc != mTarget) {
        int dir = mLoc < mTarget ?
			 (mIsBackwards ? BACKWARD : FORWARD) :
	                 (mIsBackwards ? FORWARD : BACKWARD);
	mLoc += mLoc < mTarget ? 1 : -1;

	//D("StepperActuator::act stepping: "); D(mName->c_str()); D(" "); DL(dir);
	m->onestep(dir, DOUBLE);

	//to see a pulse on the scope, enable "5" as an output and uncomment:
	//const PinDescription &p = g_APinDescription[5];
	//PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
	
	if (mLoc == mTarget) {
	    D("StepperActuator::processCommand; "); D(mName->c_str()); DL(";Stopped.");
	    m->release();
	    mRunning = false;

	    // remove this StepperActuator from the ISR handler list
	    int i = 0;
	    while ((s_steppers[i] != this) && s_steppers[i]) 
	        i++;
	    if (s_steppers[i]) {
	        PL("Removing stepper from ISR handler list");
	        int j = 0;
		while (s_steppers[j])
		    j++;
		j--; // went one too far
		if (i == j) {
		    P("Removing item "); P(i); PL(" from ISR handler list (i==j)");
		    s_steppers[i] = NULL;
		} else {
		    P("Moving item "); P(j); P(" down to "); P(i); PL(" on ISR handler list");
		    s_steppers[i] = s_steppers[j];
		    s_steppers[j] = NULL;
		}
	    } else {
	        PL("Stepper wasn't found on the ISR handler list!?!?");
	    }
	    
	    PL("Released the stepper motor");
	}
    }
}


bool StepperActuator::isItTimeYet(unsigned long now)
{
    return false;
}


bool StepperActuator::isItTimeYetForSelfDrive(unsigned long now)
{
    if ((now > mNextActionTime) && (mLoc != mTarget)) {
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


Str StepperActuator::TAG(const char *memberfunc, const char *msg) const
{
    Str tag = "StepperActuator(";
    tag.append(*mName);
    tag.append(")::");
    tag.append(memberfunc);
    tag.append("; ");
    tag.append(msg);
    return tag;
}


bool StepperActuator::isMyCommand(const char *msg) const
{
    DL("StepperActuator::isMyResponse");
    const char *token = "action|";
    if (strncmp(msg, token, strlen(token)) == 0) {
        msg += strlen(token);
	if (strncmp(msg, mName->c_str(), mName->len()) == 0) {
	    msg += mName->len();
	    token = "-target|";
	    return (strncmp(msg, token, strlen(token)) == 0);
	}
    } 
    return false;
}


const char *StepperActuator::processCommand(const char *msg)
{
    const char *token0 = "action|";
    const char *token1 = mName->c_str();
    const char *token2 = "-target|";
    Str command(msg + strlen(token0) + strlen(token1) + strlen(token2));

    D(TAG("processCommand", "parsing: ").c_str()); DL(command.c_str());
    int newTarget = atoi(command.c_str());
    if (newTarget != mTarget) {
        D(TAG("processCommand", "mTarget was ").c_str()); DL(mTarget);
	D(TAG("processCommand", "new target is ").c_str()); DL(newTarget);
        mTarget = newTarget;
        if (mTarget != mLoc) {
	    D("StepperActuator::processCommand; "); D(mName->c_str()); DL("; Running...");

	    // put this StepperActuator on the ISR handler list
	    // (consider that it might already be there)
	    bool found = false;
	    int i = 0;
	    while (!found && s_steppers[i]) {
	        found = s_steppers[i++] == this;
	    }
	    if (!found) {
	        D("adding stepper to ISR list at entry "); D(i); DL("");
	        s_steppers[i] = this;
	    } else {
	        PL("stepper already on the list!?!?");
	    }
	    
	    scheduleNextAction(millis());
	    
	    mRunning = true;
	}
    }

    const char *p = Parse::consumeToEOL(command.c_str());
    return p;
}


