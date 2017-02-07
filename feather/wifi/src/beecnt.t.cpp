#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>


#include <hiveconfig.h>
#include <hive_platform.h>
#include <RateProvider.h>
#include <Timestamp.h>
#include <Mutex.h>
#include <beecnt.h>
#include <str.h>


#define VERSION "beecnt.dev"

#define INIT 0
#define SENSOR_SAMPLE           (INIT+1)
#define LOOP                    SENSOR_SAMPLE

static int s_mainState = INIT;
static Mutex sWifiMutex;
static HiveConfig *s_config = NULL;
static RateProvider *s_rateProvider = NULL;
static BeeCounter *s_beeCounter = NULL;
static Timestamp *s_timestamp = NULL;


#define BEECNT_PLOAD_PIN        10
#define BEECNT_CLOCK_PIN         9
#define BEECNT_DATA_PIN         11


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

    s_config = new HiveConfig(rcause, VERSION);
    s_config->setDefault();
    PL("Using this config: ");
    s_config->print();
}


class MyRateProvider : public RateProvider {
public: 
  int secondsBetweenSamples() const {return 10;}
};




void loop(void)
{
    unsigned long now = millis();
    switch (s_mainState) {
    case INIT: {
	s_timestamp = new Timestamp(s_config->getSSID(), s_config->getPSWD(),
				    s_config->getDbHost(), s_config->getDbPort(),
				    s_config->isSSL(), s_config->getDbCredentials());
        s_rateProvider = new MyRateProvider();
	s_beeCounter = new BeeCounter(*s_config, "bee", *s_rateProvider, *s_timestamp, now,
				      BEECNT_PLOAD_PIN, BEECNT_CLOCK_PIN, BEECNT_DATA_PIN);

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(s_beeCounter->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_20K_init();
	PL("PulseGenerators initialized;");
	
	s_mainState = LOOP;
    }
      break;
      
    case SENSOR_SAMPLE: {
        TF("Sensor sample state");
	HivePlatform::nonConstSingleton()->clearWDT();
	
	// see if it's time to sample
	if (s_beeCounter->isItTimeYet(now)) {
	    TF("beecounter says it's time");
	    TRACE("calling loop");
	    s_beeCounter->loop(now, &sWifiMutex);

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
