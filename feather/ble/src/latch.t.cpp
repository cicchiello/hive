#include <Arduino.h>

//#define HEADLESS
#define NDEBUG


#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) do {} while (0)
#define PL(args) do {} while (0)
#endif

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#ifndef NDEBUG
#define assert(c,msg) if (!(c)) {PL("ASSERT"); HivePlatform::singleton()->trace(msg); while(1);}
#else
#define assert(c,msg) do {} while(0);
#endif


#include <hive_platform.h>
#include <TempSensor.h>
#include <RateProvider.h>
#include <ServoConfig.h>
#include <latch.h>
#include <str.h>


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
  
#define LATCH_SERVO_PIN        12

#define INIT 0
#define SENSOR_SAMPLE           (INIT+1)
#define LOOP                    SENSOR_SAMPLE

static int s_mainState = INIT;
static RateProvider *s_rateProvider = NULL;
static Latch *s_latch = NULL;
static TempSensor *s_temp = NULL;
static ServoConfig *s_config = NULL;


void loop(void)
{
    unsigned long now = millis();
    switch (s_mainState) {
    case INIT: {
        s_rateProvider = new MyRateProvider();
	s_temp = new TempSensor("temp", *s_rateProvider, now);
	s_config = new MyServoConfig();
	s_latch = new Latch("latch", *s_rateProvider, now, LATCH_SERVO_PIN, *s_temp, *s_config);

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_20K(s_latch->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_20K_init();
	PL("PulseGenerators initialized;");
	
	s_mainState = LOOP;
    }
      break;
      
    case SENSOR_SAMPLE: {
        HivePlatform::singleton()->trace("Sensor sample state");
	HivePlatform::nonConstSingleton()->clearWDT();
	
	// see if it's time to sample
	if (s_latch->isItTimeYet(now)) {
	    HivePlatform::singleton()->trace("starting sampling sensor");
	    DL("Starting sampling sensor");
	    s_latch->scheduleNextSample(now);

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
