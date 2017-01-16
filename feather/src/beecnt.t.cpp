
#include <Arduino.h>

#define HEADLESS
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
#include <RateProvider.h>
#include <beecnt.h>
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

    delay(500);

    PL("Bee Counter debug console");
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



#define INIT 0
#define SENSOR_SAMPLE           (INIT+1)
#define LOOP                    SENSOR_SAMPLE

static int s_mainState = INIT;
static RateProvider *s_rateProvider = NULL;
static BeeCounter *s_beeCounter = NULL;


#define BEECNT_PLOAD_PIN        10
#define BEECNT_CLOCK_PIN         9
#define BEECNT_DATA_PIN         11

void loop(void)
{
    unsigned long now = millis();
    switch (s_mainState) {
    case INIT: {
        s_rateProvider = new MyRateProvider();
	s_beeCounter = new BeeCounter("bee", *s_rateProvider, now,
				      BEECNT_PLOAD_PIN, BEECNT_CLOCK_PIN, BEECNT_DATA_PIN);

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(s_beeCounter->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_10K_init();
	PL("PulseGenerators initialized;");
	
	s_mainState = LOOP;
    }
      break;
      
    case SENSOR_SAMPLE: {
        HivePlatform::singleton()->trace("Sensor sample state");
	HivePlatform::nonConstSingleton()->clearWDT();
	
	// see if it's time to sample
	if (s_beeCounter->isItTimeYet(now)) {
	    HivePlatform::singleton()->trace("starting sampling sensor");
	    DL("Starting sampling sensor");
	    s_beeCounter->scheduleNextSample(now);

	    Str sensorValueStr;
	    if (s_beeCounter->sensorSample(&sensorValueStr)) {
	        P("Sampled bee counter: ");
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
