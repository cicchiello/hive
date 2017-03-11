#include <StepperActuator2.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>


#include <hiveconfig.h>

#include <Wire.h>
#if defined(ARDUINO_SAM_DUE)
 #define WIRE Wire1
#else
 #define WIRE Wire
#endif

#include <hive_platform.h>

#include <str.h>
#include <strutils.h>

//#include <StepperPWM.h>


class PWM {
public:
  PWM();
  ~PWM() {}
	
  void start(int address, int port, bool isBackwards);
  void stop(int address, int port);

  unsigned long usPer4Steps() {return mUsPer4Steps;}
  int msPer4Steps() {return usPer4Steps()/1000;}

  unsigned long mUsPer4Steps;
};

static PWM sPWM;


class StepperActuator2PulseGenConsumer : public PulseGenConsumer {
private:
  StepperActuator2PulseGenConsumer();

  StepperActuator2 **mSteppers;

public:
  static StepperActuator2PulseGenConsumer *nonConstSingleton();

  void addStepper(StepperActuator2 *stepper);
  void removeStepper(StepperActuator2 *stepper);

  void pulse(unsigned long now);
};


void StepperActuator2PulseGenConsumer::addStepper(StepperActuator2 *stepper)
{
    TF("StepperActuator2PulseGenConsumer::addStepper");

    // if there are no steppers currently on the ISR handler list, then let's start the pulse generator
    if (mSteppers[0] == NULL) {
        PulseGenConsumer *consumer = StepperActuator2PulseGenConsumer::nonConstSingleton();
        HivePlatform::nonConstSingleton()->registerPulseGenConsumer_11K(consumer);
    }
    
    // put given StepperActuator2 on the ISR handler list
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


void StepperActuator2PulseGenConsumer::removeStepper(StepperActuator2 *stepper)
{
    // remove given StepperActuator2 from the ISR handler list
    TF("StepperActuator2PulseGenConsumer::removeStepper");
    
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
        PulseGenConsumer *consumer = StepperActuator2PulseGenConsumer::nonConstSingleton();
        HivePlatform::nonConstSingleton()->unregisterPulseGenConsumer_11K(consumer);
    }
}


StepperActuator2PulseGenConsumer::StepperActuator2PulseGenConsumer()
{
    mSteppers = new StepperActuator2*[10];
    for (int i = 0; i < 10; i++)
        mSteppers[i] = NULL;
}

/* STATIC */
StepperActuator2PulseGenConsumer *StepperActuator2PulseGenConsumer::nonConstSingleton()
{
    static StepperActuator2PulseGenConsumer sSingleton;
    return &sSingleton;
}


void StepperActuator2PulseGenConsumer::pulse(unsigned long now)
{
    TF("StepperActuator2PulseGenConsumer::pulse");
    
    //to see the pulse on the scope, enable "5" as an output and uncomment:
    //const PinDescription &p = g_APinDescription[5];
    //PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
    
    bool didSomething = true;
    while (didSomething) {
        didSomething = false;
	int i = 0;
	while (!didSomething && mSteppers[i]) {
	    if (mSteppers[i]->isItTimeYetForSelfDrive(now)) {

	        mSteppers[i]->scheduleNext4Steps(now); // must be done before step!
		// (step may remove the stepper from this list we're iterating!)

	        mSteppers[i]->step();
		
		now = millis();
		didSomething = true;
	    }
	    i++;
	}
    }
}





PulseGenConsumer *StepperActuator2::getPulseGenConsumer()
{
    return StepperActuator2PulseGenConsumer::nonConstSingleton();
}

StepperActuator2::StepperActuator2(const HiveConfig &config,
				 const RateProvider &rateProvider,
				 const char *name,
				 unsigned long now,
				 int address, int port,
				 bool isBackwards)
  : Actuator(name, now), mLoc(0), mTarget(0), 
    mRunning(false), mIsBackwards(isBackwards), mPort(port), mAddress(address)
{
    TF("StepperActuator2::StepperActuator2");
}


StepperActuator2::~StepperActuator2()
{
    TF("StepperActuator2::~StepperActuator2");
}

void StepperActuator2::step()
{
    TF("StepperActuator2::step");

    if (mLoc != mTarget) {
        unsigned long now_us = micros();
	unsigned long runtimeSoFar_us = now_us - mUsStartTime;
	bool done = runtimeSoFar_us >= mDurationOfRun_us;

	mLoc = (int) (mTarget*((double)runtimeSoFar_us/(double)mDurationOfRun_us));

	//to see a pulse on the scope, enable "5" as an output and uncomment:
	//const PinDescription &p = g_APinDescription[5];
	//PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;

	if (done) {
	    sPWM.stop(mAddress, mPort);
	    PH("Stopped");
	    
	    mRunning = false;

	    // remove this StepperActuator2 from the ISR handler list
	    StepperActuator2PulseGenConsumer::nonConstSingleton()->removeStepper(this);

	    mLoc = mTarget = 0;
	    TRACE("Released the stepper motor");
	}
    }
}


bool StepperActuator2::isItTimeYetForSelfDrive(unsigned long now)
{
    TF("StepperActuator2::isItTimeYetForSelfDrive");
    unsigned long when = getNextActionTime();
    if ((now > when) && (mLoc != mTarget)) {
        TRACE4("StepperActuator2::isItTimeYetForSelfDrive; missed an appointed visit by ",
	       now - when, " ms; now == ", now);
    }
    
    return now >= when;
}


bool StepperActuator2::loop(unsigned long now)
{
    return mLoc != mTarget;
}


void StepperActuator2::processMsg(unsigned long now, const char *msg)
{
    static const char *InvalidMsg = "invalid msg; did isMyMsg return true?";
    
    TF("StepperActuator2::processMsg");

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

        mTarget = newTarget;
        if (mTarget != mLoc) {
	    PH3("Running for ", (mTarget-mLoc), " steps");

	    // put this StepperActuator2 on the ISR handler list
	    // (consider that it might already be there)
	    StepperActuator2PulseGenConsumer::nonConstSingleton()->addStepper(this);

	    TRACE("Calling StepperPWMstart");
	    sPWM.start(mAddress, mPort, mIsBackwards);
	    mUsStartTime = micros();
	    mDurationOfRun_us = newTarget/4*sPWM.usPer4Steps();
	    TRACE2("mDurationofRun_us: ", mDurationOfRun_us);
	
	    scheduleNext4Steps(millis());
	    
	    mRunning = true;
	}
    }
}


bool StepperActuator2::isMyMsg(const char *msg) const
{
    TF("StepperActuator2::isMyMsg");
    const char *name = getName();
    TRACE2("isMyMsg for motor: ", name);
    TRACE2("Considering msg: ", msg);
    
    if (strncmp(msg, name, strlen(name)) == 0) {
        msg += strlen(name);
	const char *token = "|";
	bool r = (strncmp(msg, token, strlen(token)) == 0) && StringUtils::isNumber(msg+strlen(token));
	if (r) {
	    TRACE("it's mine!");
	}
	return r;
    } else {
        return false;
    }
}

void StepperActuator2::scheduleNext4Steps(unsigned long now)
{
  setNextActionTime(now + sPWM.msPer4Steps());
}


static const int StepsPerRevolution = 200;
static const int StepsPerPWMCycle = 4;

static const int PWMA = 2;
static const int PWMB = 7;
static const int AIN1 = 4;
static const int AIN2 = 3;
static const int BIN1 = 5;
static const int BIN2 = 6;

static const int i2caddr = 0x60;

static const int PCA9685_MODE1 = 0x0;
static const int PCA9685_PRESCALE = 0xFE;
static const int LED0_ON_L = 0x6;

static void write8(uint8_t addr, uint8_t d) {
  WIRE.beginTransmission(i2caddr);
#if ARDUINO >= 100
  WIRE.write(addr);
  WIRE.write(d);
#else
  WIRE.send(addr);
  WIRE.send(d);
#endif
  WIRE.endTransmission();
}


static uint8_t read8(uint8_t addr) {
  WIRE.beginTransmission(i2caddr);
#if ARDUINO >= 100
  WIRE.write(addr);
#else
  WIRE.send(addr);
#endif
  WIRE.endTransmission();

  WIRE.requestFrom((uint8_t)i2caddr, (uint8_t)1);
#if ARDUINO >= 100
  return WIRE.read();
#else
  return WIRE.receive();
#endif
}


static void setPWM(uint8_t num, uint16_t on, uint16_t off) {
  TF("::setPWM");
  TRACE2("Setting PWM ",num);
  TRACE3(on, "->", off);

  WIRE.beginTransmission(i2caddr);
#if ARDUINO >= 100
  WIRE.write(LED0_ON_L+4*num);
  WIRE.write(on);
  WIRE.write(on>>8);
  WIRE.write(off);
  WIRE.write(off>>8);
#else
  WIRE.send(LED0_ON_L+4*num);
  WIRE.send((uint8_t)on);
  WIRE.send((uint8_t)(on>>8));
  WIRE.send((uint8_t)off);
  WIRE.send((uint8_t)(off>>8));
#endif
  WIRE.endTransmission();
}


PWM::PWM()
{
    TF("PWM::PWM");
  
    WIRE.begin();
  
    write8(PCA9685_MODE1, 0x0); // reset
  
    uint8_t prescale = 123;
    TRACE2("using prescale of ", prescale);

    uint8_t oldmode = read8(PCA9685_MODE1);
    uint8_t newmode = (oldmode&0x7F) | 0x10; // sleep
    write8(PCA9685_MODE1, newmode); // go to sleep
    write8(PCA9685_PRESCALE, prescale); // set the prescaler
    delay(1);                           // allow 500us for the oscillator to settle
    write8(PCA9685_MODE1, oldmode);
    delay(1);
    write8(PCA9685_MODE1, oldmode | 0xa1);  // This resets and sets the MODE1 register to turn on auto increment.
                                            // This is why the beginTransmission below was not working.
  
    for (uint8_t i=0; i<16; i++) 
        setPWM(i, 0, 0);

    // set speed
    TRACE2("Steps per revolution: ", StepsPerRevolution);

    double secondsPer4Steps = 1.0/(25000000.0/4096/prescale);
    double usPerS = 1000000;
    sPWM.mUsPer4Steps = secondsPer4Steps*usPerS;
  
    TRACE2("us Per 4 Steps: ", mUsPer4Steps);
}


void PWM::stop(int address, int port)
{
    setPWM(AIN1, 0, 0);
    setPWM(AIN2, 0, 0);
    setPWM(BIN1, 0, 0);
    setPWM(BIN2, 0, 0);
    setPWM(PWMA, 0, 0);
    setPWM(PWMB, 0, 0);
}


void PWM::start(int address, int port, bool isBackwards)
{
    TF("PWM::start");
    assert(address == 0x60, "only address==0x60 supported for now"); 
    assert(port == 1, "only port==1 supported for now");
  
    setPWM(PWMA, 4096, 0);
    setPWM(PWMB, 4096, 0);

    if (isBackwards) {
        // runs CCW at a little slower than 1 rev per second
        setPWM(AIN1, 0x355, 0xc00);
	setPWM(AIN2, 0xc00, 0x3ff);
	setPWM(BIN1, 0x7ff, 0xfff);
	setPWM(BIN2, 0x0, 0x7ff);
    } else {
        // runs CW at a little slower than 1 rev per second
        setPWM(AIN1, 0x0, 0x7ff);
	setPWM(AIN2, 0x7ff, 0xfff);
	setPWM(BIN1, 0xc00, 0x3ff);
	setPWM(BIN2, 0x355, 0xc00);
    }
}

