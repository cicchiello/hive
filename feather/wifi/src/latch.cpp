#include <latch.h>

#include <Arduino.h>

#include <str.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <TempSensor.h>
#include <ServoConfig.h>
#include <platformutils.h>


class LocalServoConfig : public ServoConfig {
private:
  double t;
  bool cw;
  int lmin,lmax;

public:
  LocalServoConfig() : t(0), cw(true), lmin(-1), lmax(-1) {}
  ~LocalServoConfig() {}
      
  void set(const ServoConfig &c) {
      t = c.getTripTemperatureC();
      cw = c.isClockwise();
      lmin = c.getLowerLimitTicks();
      lmax = c.getUpperLimitTicks();
  }
      
  bool equals(const ServoConfig &o) const {
    return (t == o.getTripTemperatureC()) &&
           (cw == o.isClockwise()) &
           (lmin == o.getLowerLimitTicks()) &&
           (lmax == o.getUpperLimitTicks());
  }
      
  double getTripTemperatureC() const {return t;}
  bool isClockwise() const {return cw;}
  int getLowerLimitTicks() const {return lmin;}
  int getUpperLimitTicks() const {return lmax;}
};


static Latch *sSingleton = 0;

Latch::Latch(const HiveConfig &config,
	     const char *name,
	     const class RateProvider &rateProvider,
	     const class TimeProvider &timeProvider,
	     unsigned long now, int servoPin,
	     const TempSensor &tempSensor, const ServoConfig &servoConfig, 
	     Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex),
    mServoPin(servoPin), mTempSensor(tempSensor),
    mServoConfigProvider(servoConfig), mCurrentConfig(new LocalServoConfig()),
    mIsHigh(false), mIsMoving(false), mIsAtClockwiseLimit(true)
{
    if (sSingleton) {
        WDT_TRACE("Only one Latch object can currently be constructed");
	while (1) {};
    }
    sSingleton = this;
      
    //initialize digital pins
    pinMode(mServoPin, OUTPUT);
}



Latch::~Latch()
{
  delete mCurrentConfig;
}




#define TEMP_SAMPLE_TIME_MS   (10*1000)                             // sample temp every xx MS
#define CYCLE_TIME_TICKS      400                                   // 20ms @ 50us per tick
#define CLKWISE_PULSE_TICKS   mCurrentConfig->getUpperLimitTicks()  // 2ms
#define CCLCKWISE_PULSE_TICKS mCurrentConfig->getLowerLimitTicks()  // 2.15ms
#define MOVE_CLOCKWISE        mCurrentConfig->isClockwise()         // move clockwise when temp >= trip temp
#define SETTLE_TIME_MS        7*1000                                // about 7 seconds
#define TRIP_TEMP_C           mCurrentConfig->getTripTemperatureC() // defaults to ~100F

void Latch::clockwise(unsigned long now)
{
    mTicksOn = CLKWISE_PULSE_TICKS;
    mTicksOff = CYCLE_TIME_TICKS-mTicksOn;
    mTicks = mTicksOff;
    mIsHigh = false;
    digitalWrite(mServoPin, LOW);
    mIsAtClockwiseLimit = true;
    mStartedMoving = now;
    mIsMoving = true;
}


void Latch::counterclockwise(unsigned long now)
{
    mTicksOn = CCLCKWISE_PULSE_TICKS;
    mTicksOff = CYCLE_TIME_TICKS-mTicksOn;
    mTicks = mTicksOff;
    mIsHigh = false;
    digitalWrite(mServoPin, LOW);
    mIsAtClockwiseLimit = false;
    mStartedMoving = now;
    mIsMoving = true;
}


void Latch::pulse(unsigned long now)
{
    // expecting to be called once every 50us
  
    //to see the pulse on the scope, enable "5" as an output and uncomment:
    const PinDescription &p = g_APinDescription[5];
    PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;

    if (mFirstPulse) {
        mFirstPulse = false;
	
        // setup a timer to sample the temperature in 30s
	mLastTempTime = now;
    } else if (now - mLastTempTime > TEMP_SAMPLE_TIME_MS) {
	mLastTempTime = now;
	if (mTempSensor.hasTemp()) {
	    DL(TAG("pulse", "Getting temperature sample").c_str());
	    double t = mTempSensor.getTemp();

//#define TESTING
#ifdef TESTING	
	    // for testing, change the temp every 20 seconds
	    unsigned long seconds = now/1000;
	    if (seconds/10 % 2 == 0) {
	        t = 40.0;
	    } else {
	        t = 35.0;
	    }
#endif
	
	    if (!isnan(t)) {
	        mTemp = t;

		D(TAG("pulse", "Considering temp: ").c_str()); DL(mTemp);
		bool configChanged = !mCurrentConfig->equals(mServoConfigProvider);
		if (configChanged)
		    mCurrentConfig->set(mServoConfigProvider);
		if (mTemp > TRIP_TEMP_C) {
		    DL(TAG("pulse", "trip point exceeded").c_str());
		    if (MOVE_CLOCKWISE) {
		        if (configChanged || !mIsAtClockwiseLimit) {
			    DL(TAG("pulse", "moving to clockwise limit").c_str());
			    clockwise(now);
			}
		    } else {
		        if (configChanged || mIsAtClockwiseLimit) {
			    DL(TAG("pulse", "moving to counter clockwise limit").c_str());
			    counterclockwise(now);
			}
		    }
		} else {
		    DL(TAG("pulse", "below trip point").c_str());
		    if (MOVE_CLOCKWISE) {
		        if (configChanged || mIsAtClockwiseLimit) {
			    DL(TAG("pulse", "moving to counter clockwise limit").c_str());
			    counterclockwise(now);
			}
		    } else {
		        if (configChanged || !mIsAtClockwiseLimit) {
			    DL(TAG("pulse", "moving to clockwise limit").c_str());
			    clockwise(now);
			}
		    }
		}
	    } else {
	        DL(TAG("pulse", "sample is NAN").c_str());
	    }
	}
    }
	
    
    if (mIsMoving) {
        if (--mTicks == 0) {
	    if (mIsHigh) {
	        digitalWrite(mServoPin, LOW);
		mIsHigh = false;
		mTicks = mTicksOff;
	    } else {
	        digitalWrite(mServoPin, HIGH);
		mIsHigh = true;
		mTicks = mTicksOn;
	    }
	}

	if (now - mStartedMoving > SETTLE_TIME_MS) {
	    digitalWrite(mServoPin, LOW);
	    mIsMoving = false;
	}
    } 
}


bool Latch::sensorSample(Str *value)
{
    *value = mIsAtClockwiseLimit ? "Clockwise" : "AntiClockwise";
    
    return true;
}


