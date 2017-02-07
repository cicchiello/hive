#include <Arduino.h>

//#define HEADLESS
//#define NDEBUG

#include <Trace.h>


#include <hiveconfig.h>
#include <hive_platform.h>
#include <TempSensor.h>
#include <RateProvider.h>
#include <Timestamp.h>
#include <Mutex.h>
#include <ServoConfig.h>
#include <latch.h>
#include <str.h>


#define VERSION "latch.dev"

#define LATCH_SERVO_PIN        12

#define INIT 0
#define SENSOR_SAMPLE           (INIT+1)
#define LOOP                    SENSOR_SAMPLE

static int s_mainState = INIT;
static Mutex sWifiMutex;
static HiveConfig *s_hiveConfig = NULL;
static RateProvider *s_rateProvider = NULL;
static Latch *s_latch = NULL;
static TempSensor *s_temp = NULL;
static ServoConfig *s_config = NULL;
static Timestamp *s_timestamp = NULL;


void setup(void)
{
    const char *rcause = HivePlatform::singleton()->getResetCause();
  
    delay(500);

#ifndef HEADLESS
    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
#endif
  
    pinMode(19, INPUT);               // used for preventing runnaway on reset
    while (digitalRead(19) == HIGH) {
        delay(500);
	PL("within the runnaway prevention loop");
    }
    PL("digitalRead(19) returned LOW");
    P("RCAUSE: "); PL(rcause);

    pinMode(5, OUTPUT);         // for debugging: used to indicate ISR invocations for pulse generator
    
    delay(500);

    PL("Latch debug console");
    PL("-------------------------");

    PL();

    DL("starting WDT");
    HivePlatform::nonConstSingleton()->startWDT();
    HivePlatform::singleton()->trace("wdt handler registered");
  
    PL();

    s_hiveConfig = new HiveConfig(rcause, VERSION);
    s_hiveConfig->setDefault();
    PL("Using this config: ");
    s_hiveConfig->print();
}


class MyRateProvider : public RateProvider {
public: 
  int secondsBetweenSamples() const {return 10;}
};

class MyServoConfig : public ServoConfig {
public: 
    virtual double getTripTemperatureC() const {return 21.5;}
    virtual bool isClockwise() const {return false;}
    virtual int getLowerLimitTicks() const {return 40;}
    virtual int getUpperLimitTicks() const {return 43;}
};
  

void loop(void)
{
    unsigned long now = millis();
    switch (s_mainState) {
    case INIT: {
	s_timestamp = new Timestamp(s_hiveConfig->getSSID(), s_hiveConfig->getPSWD(),
				    s_hiveConfig->getDbHost(), s_hiveConfig->getDbPort(),
				    s_hiveConfig->isSSL(), s_hiveConfig->getDbCredentials());
        s_rateProvider = new MyRateProvider();
	s_temp = new TempSensor(*s_hiveConfig, "temp", *s_rateProvider, *s_timestamp, now);
	s_config = new MyServoConfig();
	s_latch = new Latch(*s_hiveConfig, "latch", *s_rateProvider, *s_timestamp, now,
			    LATCH_SERVO_PIN, *s_temp, *s_config);

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_20K(s_latch->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_20K_init();
	PL("PulseGenerators initialized;");
	
	s_mainState = LOOP;
    }
      break;
      
    case SENSOR_SAMPLE: {
        TF("Sensor sample state");
	HivePlatform::nonConstSingleton()->clearWDT();
	
	// see if it's time to sample
	if (s_latch->isItTimeYet(now)) {
	    TF("latch says it's time");
	    TRACE("calling loop");
	    s_latch->loop(now, &sWifiMutex);

	    Str sensorValueStr;
	    if (s_latch->sensorSample(&sensorValueStr)) {
	        P("Sampled Latch: ");
		PL(sensorValueStr.c_str());
	    }
	}
	s_mainState = LOOP;
    }
      break;

    default:
      HivePlatform::singleton()->error("Unknown s_mainState in loop; quitting");
    };
}
