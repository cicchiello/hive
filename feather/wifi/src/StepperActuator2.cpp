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

#include <platformutils.h>
#include <hive_platform.h>

#include <str.h>
#include <strutils.h>


class PWM {
private:
  uint8_t _i2caddr;
public:
  static const int PWM_BASE_I2CADDR = 0x60;
  static const int MAX_PWM = 32;
  
  PWM(uint8_t i2caddr);
  ~PWM() {}
	
  void start(int port, bool isCCW);
  void stop(int port);
  void stopAll();

  unsigned long usPer4Steps() {return mUsPer4Steps;}

  unsigned long mUsPer4Steps;
};


static PWM *sPWM[PWM::MAX_PWM];
static bool sPWMsInitialized = false;



class StepperActuator2PulseGenConsumer : public PulseGenConsumer {
private:
  StepperActuator2PulseGenConsumer();

  StepperActuator2 **mSteppers;
  unsigned long *mNumPulses;

public:
  static StepperActuator2PulseGenConsumer *nonConstSingleton();

  int addStepper(StepperActuator2 *stepper);
  void removeStepper(StepperActuator2 *stepper);

  void startTimer(int index, unsigned long numPulses);
  
  void stopAll();
  
  void pulse(unsigned long now);
};


int StepperActuator2PulseGenConsumer::addStepper(StepperActuator2 *stepper)
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
    mNumPulses[i] = 0;
    return i;
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
	mNumPulses[i] = 0;
    } else {
        mSteppers[i] = mSteppers[j];
	mNumPulses[i] = mNumPulses[j];
	mSteppers[j] = NULL;
	mNumPulses[j] = 0;
    }

    // if the ISR Handler list is empty, stop the pulse generator
    if (mSteppers[0] == NULL) {
        PulseGenConsumer *consumer = StepperActuator2PulseGenConsumer::nonConstSingleton();
        HivePlatform::nonConstSingleton()->unregisterPulseGenConsumer_11K(consumer);
    }
}


void StepperActuator2PulseGenConsumer::startTimer(int index, unsigned long numPulses)
{
    bool found = false;
    int i = 0;
    while (!found && mSteppers[i]) {
        found |= mNumPulses[i] > 0;
        i++;
    }

    mNumPulses[index] = numPulses;
};


StepperActuator2PulseGenConsumer::StepperActuator2PulseGenConsumer()
{
    mSteppers = new StepperActuator2*[10];
    mNumPulses = new unsigned long[10];
    for (int i = 0; i < 10; i++) {
        mSteppers[i] = NULL;
	mNumPulses[i] = 0;
    }
}


/* STATIC */
StepperActuator2PulseGenConsumer *StepperActuator2PulseGenConsumer::nonConstSingleton()
{
    static StepperActuator2PulseGenConsumer sSingleton;
    return &sSingleton;
}


void StepperActuator2PulseGenConsumer::pulse(unsigned long now)
{
    //to see the pulse on the scope, enable "5" as an output and uncomment:
    const PinDescription &p = g_APinDescription[5];
    PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;

    for (int i = 0; mSteppers[i]; i++)
        if(mNumPulses[i] && (--mNumPulses[i] == 0)) {
	    StepperActuator2 *stepper = mSteppers[i];
	    sPWM[stepper->mI2Caddr-PWM::PWM_BASE_I2CADDR]->stop(stepper->mPort);
	    stepper->mRunning = false;
	    stepper->mTarget = 0;
	}
}


void StepperActuator2PulseGenConsumer::stopAll()
{
    for (int i = 0; mSteppers[i]; i++) {
        if (mNumPulses[i]) {
	    StepperActuator2 *stepper = mSteppers[i];
	    mNumPulses[i] = 0;
	    sPWM[stepper->mI2Caddr-PWM::PWM_BASE_I2CADDR]->stop(stepper->mPort);
	    stepper->mRunning = false;
	    stepper->mTarget = 0;
	}
    }
}



static PlatformUtils::WDT_EarlyWarning_Func sDaisyChainedFunc = 0;
static void FatalShutdown()
{
    StepperActuator2PulseGenConsumer::nonConstSingleton()->stopAll();
    sDaisyChainedFunc();
}



StepperActuator2::StepperActuator2(const HiveConfig &config,
				   const RateProvider &rateProvider,
				   const char *name,
				   unsigned long now,
				   int i2caddr, int port,
				   bool isBackwards)
  : Actuator(name, now), mTarget(0), 
    mRunning(false), mIsBackwards(isBackwards), mPort(port), mI2Caddr(i2caddr)
{
    TF("StepperActuator2::StepperActuator2");

    if (!sPWMsInitialized) {
	sDaisyChainedFunc = PlatformUtils::nonConstSingleton().initWDT(FatalShutdown);

	for (int i = 0; i < PWM::MAX_PWM; i++)
	    sPWM[i] = NULL;
	
        sPWMsInitialized = true;
    }

    if (sPWM[i2caddr-PWM::PWM_BASE_I2CADDR] == NULL)
        sPWM[i2caddr-PWM::PWM_BASE_I2CADDR] = new PWM(i2caddr);

    mStepperIndex = StepperActuator2PulseGenConsumer::nonConstSingleton()->addStepper(this);
}


StepperActuator2::~StepperActuator2()
{
    TF("StepperActuator2::~StepperActuator2");
    StepperActuator2PulseGenConsumer::nonConstSingleton()->stopAll();
}


bool StepperActuator2::loop(unsigned long now)
{
    return mTarget != 0;
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
        if (mTarget != 0) {
	    PH3("Running for ", mTarget, " steps");

	    int newTargetAbs = newTarget > 0 ? newTarget : -newTarget;
	    unsigned long durationOfRun_us = newTargetAbs/4*sPWM[mI2Caddr-PWM::PWM_BASE_I2CADDR]->usPer4Steps();
	    double durationOfRun_s = durationOfRun_us / 1000000.0;
	    double pulsesPerSecond = HivePlatform::SAMPLES_PER_SECOND_11K*0.9565;
	    unsigned long pulsesForRun = (unsigned long) (durationOfRun_s*pulsesPerSecond);

	    TRACE2("usPer4Steps(): ", sPWM[mI2Caddr-PWM::PWM_BASE_I2CADDR]->usPer4Steps());
	    TRACE2("pulsesPerSecond: ", pulsesPerSecond);
	    TRACE2("durationOfRun_us: ", durationOfRun_us);
	    TRACE2("mDurationOfRun_s: ", durationOfRun_s);
	    TRACE2("pulsesForRun: ", pulsesForRun);
	    
	    StepperActuator2PulseGenConsumer::nonConstSingleton()->startTimer(mStepperIndex, pulsesForRun);

	    TRACE("Calling PWM->start");
	    sPWM[mI2Caddr-PWM::PWM_BASE_I2CADDR]->start(mPort, newTarget > 0 ? mIsBackwards : !mIsBackwards);
	
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



static const int StepsPerRevolution = 200;
static const int StepsPerPWMCycle = 4;

static const int PCA9685_MODE1 = 0x0;
static const int PCA9685_PRESCALE = 0xFE;
static const int LED0_ON_L = 0x6;

static void write8(uint8_t i2caddr, uint8_t addr, uint8_t d) {
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


static uint8_t read8(uint8_t i2caddr, uint8_t addr) {
  WIRE.beginTransmission(i2caddr);
#if ARDUINO >= 100
  WIRE.write(addr);
#else
  WIRE.send(addr);
#endif
  WIRE.endTransmission();

  WIRE.requestFrom(i2caddr, (uint8_t)1);
#if ARDUINO >= 100
  return WIRE.read();
#else
  return WIRE.receive();
#endif
}


static void setPWM(uint8_t i2caddr, uint8_t num, uint16_t on, uint16_t off) {
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


PWM::PWM(uint8_t i2caddr)
  : _i2caddr(i2caddr)
{
    TF("PWM::PWM");
  
    WIRE.begin();
  
    write8(i2caddr, PCA9685_MODE1, 0x0); // reset
  
    uint8_t prescale = 121;
    TRACE2("using prescale of ", prescale);

    uint8_t oldmode = read8(i2caddr, PCA9685_MODE1);
    uint8_t newmode = (oldmode&0x7F) | 0x10; // sleep
    write8(i2caddr, PCA9685_MODE1, newmode); // go to sleep
    write8(i2caddr, PCA9685_PRESCALE, prescale); // set the prescaler
    delay(1);                           // allow 500us for the oscillator to settle
    write8(i2caddr, PCA9685_MODE1, oldmode);
    delay(1);
    write8(i2caddr, PCA9685_MODE1, oldmode | 0xa1);  // This resets and sets the MODE1 register to turn on auto increment.
                                            // This is why the beginTransmission below was not working.
  
    for (uint8_t i=0; i<16; i++) 
        setPWM(i2caddr, i, 0, 0);

    // set speed
    TRACE2("Steps per revolution: ", StepsPerRevolution);

    mUsPer4Steps = (unsigned long) (4096.0*prescale/25.0 + 0.5);
    
    TRACE2("prescale: ", prescale);
    TRACE2("us Per 4 Steps: ", mUsPer4Steps);
}


void PWM::stop(int port)
{
    TF("PWM::stop");
    PH("Stopping");
    
    int pwma, ain2, ain1, pwmb, bin2, bin1;
    switch (port) {
    case 1: 
      pwma = 8; ain2 = 9; ain1 = 10;
      pwmb = 13; bin2 = 12; bin1 = 11;
      break;
    case 2:
      pwma = 2; ain2 = 3; ain1 = 4;
      pwmb = 7; bin2 = 6; bin1 = 5;
      break;
    }
    
    setPWM(_i2caddr, ain1, 0, 0);
    setPWM(_i2caddr, ain2, 0, 0);
    setPWM(_i2caddr, bin1, 0, 0);
    setPWM(_i2caddr, bin2, 0, 0);
    setPWM(_i2caddr, pwma, 0, 0);
    setPWM(_i2caddr, pwmb, 0, 0);
}


void PWM::start(int port, bool isCCW)
{
    TF("PWM::start");
    PH4("Starting; i2caddr: ", _i2caddr, ", port: ", port);
  
    int pwma, ain2, ain1, pwmb, bin2, bin1;
    switch (port) {
    case 1: 
      pwma = 8; ain2 = 9; ain1 = 10;
      pwmb = 13; bin2 = 12; bin1 = 11;
      break;
    case 2:
      pwma = 2; ain2 = 3; ain1 = 4;
      pwmb = 7; bin2 = 6; bin1 = 5;
      break;
    }
    
    setPWM(_i2caddr, pwma, 4096, 0);
    setPWM(_i2caddr, pwmb, 4096, 0);

    if (isCCW) {
        // runs CCW 
        setPWM(_i2caddr, ain1, 0x355, 0xc00);
	setPWM(_i2caddr, ain2, 0xc00, 0x3ff);
	setPWM(_i2caddr, bin1, 0x7ff, 0xfff);
	setPWM(_i2caddr, bin2, 0x0, 0x7ff);
    } else {
        // runs CW 
        setPWM(_i2caddr, ain1, 0x0, 0x7ff);
	setPWM(_i2caddr, ain2, 0x7ff, 0xfff);
	setPWM(_i2caddr, bin1, 0xc00, 0x3ff);
	setPWM(_i2caddr, bin2, 0x355, 0xc00);
    }
}

