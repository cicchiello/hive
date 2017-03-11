#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#define HEADLESS
#define NDEBUG

#include <Trace.h>


#include <hiveconfig.h>
#include <hive_platform.h>
#include <Timestamp.h>
#include <RateProvider.h>
#include <AudioActuator.h>
#include <Mutex.h>

#include <Actuator.h>


#define VERSION "audio.dev"

#define INIT 0
#define SENSOR_SAMPLE           (INIT+1)
#define LOOP                    SENSOR_SAMPLE


static int s_mainState = INIT;
static Mutex sWifiMutex;
static HiveConfig *s_hiveConfig = NULL;
static RateProvider *s_rateProvider = NULL;
static AudioActuator *s_audio = NULL;
static Timestamp *s_timestamp = NULL;




/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
    TF("::setup");

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


/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/

void loop(void)
{
    TF("::loop");
    
    unsigned long now = millis();
    switch (s_mainState) {
    case INIT: {
        TRACE("Initializing sensors and actuators");
	s_timestamp = new Timestamp(s_hiveConfig->getSSID(), s_hiveConfig->getPSWD(),
				    s_hiveConfig->getDbHost(), s_hiveConfig->getDbPort(),
				    s_hiveConfig->isSSL(), s_hiveConfig->getDbCredentials());
        s_rateProvider = new MyRateProvider();
	s_audio = new AudioActuator("mic", now);
	
	TRACE("Sensors and Actuators initialized;");

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_20K(s_audio->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_20K_init();
	PL("PulseGenerators initialized;");
	
	// wait a bit for comms to settle
	delay(500);
	TRACE("Initializations done;");
	
	s_mainState = LOOP;
    }
      break;
      
    case SENSOR_SAMPLE: {
        TF("::loop; SENSOR_SAMPLE");
        if (s_audio->isItTimeYet(now)) 
	    s_audio->loop(now, &sWifiMutex);

	s_mainState = LOOP;
    }
      break;
      
    }
    
}

