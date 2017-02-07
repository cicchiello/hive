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

StepperActuator::StepperActuator(const HiveConfig &config,
				 const char *name,
				 unsigned long now,
				 int address, int port,
				 bool isBackwards)
  : ActuatorBase(config, name, now), mLoc(0), mTarget(0),
    mRunning(false), mIsBackwards(isBackwards)
{
    TF("StepperActuator::StepperActuator");
    TRACE("entry");
    
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
    
    TRACE("exit");
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
	    TRACE(TAG("processCommand", "Stopped.").c_str());
	    m->release();
	    mRunning = false;

	    // remove this StepperActuator from the ISR handler list
	    StepperActuatorPulseGenConsumer::nonConstSingleton()->removeStepper(this);

	    TRACE(TAG("processCommand", "Released the stepper motor").c_str());
	}
    }
}


bool StepperActuator::isItTimeYetForSelfDrive(unsigned long now)
{
    TF("StepperActuator::isItTimeYetForSelfDrive");
    if ((now > getNextActionTime()) && (mLoc != mTarget)) {
        D("StepperActuator::isItTimeYet; missed an appointed visit by ");
	D(now - getNextActionTime());
	D(" ms; now == ");
	DL(now);
    }
    
    return now >= getNextActionTime();
}


class StepperActuatorGetter : public ActuatorBase::Getter {
private:
    bool mHasTarget;
    int mTarget, mParsed;
  
public:
    StepperActuatorGetter(const char *ssid, const char *pswd, const char *dbHost, int dbPort, bool isSSL,
			  const char *url, const char *credentials)
      : ActuatorBase::Getter(ssid, pswd, dbHost, dbPort, url, credentials, isSSL),
	mHasTarget(false), mTarget(0), mParsed(false)
    {
        TF("StepperActuatorGetter::StepperActuatorGetter");
	TRACE("entry");
    }

    int getTarget() const {return mTarget;}
  
    bool hasResult() const {
        if (mParsed)
	    return mHasTarget;

	StepperActuatorGetter *nonConstThis = (StepperActuatorGetter*) this;
	nonConstThis->mParsed = true;
        CouchUtils::Doc doc;
	const CouchUtils::Doc *record = getSingleRecord(&doc);
	if (record != NULL) {
	    int ival = record->lookup("value");
	    if ((ival >= 0) &&
		(*record)[ival].getValue().isArr() &&
		((*record)[ival].getValue().getArr().getSz()==4)) {
	        const CouchUtils::Arr &val = (*record)[ival].getValue().getArr();
		if (!val[3].isDoc() && !val[3].isArr()) {
		    const Str &locStr = val[3].getStr();
		    nonConstThis->mTarget = atoi(locStr.c_str());
		    nonConstThis->mHasTarget = true;
		}
	    }
	}
	
	return mHasTarget;
    }

    static const char *ClassName() {return "StepperActuatorGetter";}
  
    const char *className() const;
};

const char *StepperActuatorGetter::className() const
{
    return StepperActuatorGetter::ClassName();
}


const void *StepperActuator::getSemaphore() const
{
    TF("StepperMonitor::getSemaphore");
    return this;
}


ActuatorBase::Getter *StepperActuator::createGetter() const
{
    TF("StepperActuator::createGetter");
    TRACE("creating getter");

    // curl -X GET 'http://jfcenterprises.cloudant.com/hive-sensor-log/_design/SensorLog/_view/by-hive-sensor?endkey=%5B%22F0-17-66-FC-5E-A1%22,%22sample-rate%22,%2200000000%22%5D&startkey=%5B%22F0-17-66-FC-5E-A1%22,%22sensor-rate%22,%2299999999%22%5D&descending=true&limit=1'
	  
    Str url, encodedUrl;
    url.append(getConfig().getDesignDocId());
    url.append("/_view/");
    url.append(getConfig().getSensorByHiveViewName());
    url.append("?endkey=[\"");
    url.append(getConfig().getHiveId());
    url.append("\",\"");
    url.append(getName());
    url.append("\",\"00000000\"]&startkey=[\"");
    url.append(getConfig().getHiveId());
    url.append("\",\"");
    url.append(getName());
    url.append("\",\"99999999\"]&descending=true&limit=1");
    CouchUtils::urlEncode(url.c_str(), &encodedUrl);
	
    url.clear();
    CouchUtils::toURL(getConfig().getLogDbName(), encodedUrl.c_str(), &url);
    TRACE2("URL: ", url.c_str());
	
    return new StepperActuatorGetter(getConfig().getSSID(), getConfig().getPSWD(),
				     getConfig().getDbHost(), getConfig().getDbPort(), getConfig().isSSL(),
				     url.c_str(), getConfig().getDbCredentials());
}


void StepperActuator::processResult(ActuatorBase::Getter *baseGetter)
{
    TF("StepperActuator::processResult");
    
    if (baseGetter->className() == StepperActuatorGetter::ClassName()) {
        StepperActuatorGetter *getter = (StepperActuatorGetter*) baseGetter;
	mTarget = getter->getTarget();
    } else {
        ERROR("class cast exception");
    }
}

