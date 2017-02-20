#include <Arduino.h>

//#define HEADLESS
//#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>
#define CHKPT(msg) HivePlatform::singleton()->trace(msg)
#define ERR(msg) HivePlatform::singleton()->error(msg)

#include <version_id.h>

#include <Provision.h>
#include <hiveconfig.h>
#include <SensorRateActuator.h>
#include <TempSensor.h>
#include <HumidSensor.h>
#include <Timestamp.h>
#include <StepperMonitor.h>
#include <StepperActuator.h>
#include <AudioUpload.h>
#include <beecnt.h>
#include <Indicator.h>
#include <Heartbeat.h>
#include <AppChannel.h>
//#include <listener.h>
#include <Mutex.h>


#include <wifiutils.h>

#define CONFIG_FILENAME         "/CONFIG.CFG"

#define INIT_SENSORS            0
#define ACQUIRE_TIMESTAMP       1
#define SENSOR_SAMPLE           (ACQUIRE_TIMESTAMP+1)
#define LOOP                    SENSOR_SAMPLE

#define BEECNT_PLOAD_PIN        10
#define BEECNT_CLOCK_PIN         9
#define BEECNT_DATA_PIN         11

static const char *ResetCause = "unknown";
static int s_mainState = INIT_SENSORS;
static bool s_isOnline = false;
static bool s_hasBeenOnline = false;

#define MAX_SENSORS 20
static Mutex sWifiMutex;
static Provision *s_provisioner = NULL;
static Indicator *s_indicator = NULL;
static AppChannel *s_appChannel = NULL;
static int s_currSensor = 0;
static Timestamp *s_timestamp = NULL;
//static Listener *s_listener = NULL;
static class Sensor *s_sensors[MAX_SENSORS];
static class Actuator *s_actuators[Actuator::MAX_ACTUATORS];

#define CNF s_provisioner->getConfig()


/**************************************************************************/
/*!
    @brief  Sets up the HW (this function is called automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
    // capture the reason for previous reset so I can inform the app
    ResetCause = HivePlatform::singleton()->getResetCause(); 
    
    delay(500);

#ifndef HEADLESS
    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
#endif
  
    P("ResetCause: "); PL(ResetCause);
    
    pinMode(19, INPUT);               // used for preventing runnaway on reset
    while (digitalRead(19) == HIGH) {
        delay(500);
	PL("within the runnaway prevention loop");
    }
    PL("digitalRead(19) returned LOW");

    //pinMode(5, OUTPUT);         // for debugging: used to indicate ISR invocations for motor drivers
    
    delay(500);
    
    PL("Hive Controller debug console");
    PL("---------------------------------------");

    HivePlatform::nonConstSingleton()->startWDT();
  
    s_provisioner = new Provision(ResetCause, VERSION, CONFIG_FILENAME, millis());

    s_indicator = new Indicator(millis());
    s_indicator->setFlashMode(Indicator::TryingToConnect);
    
    CHKPT("setup done");
    PL("Setup done...");
    
    pinMode(10, OUTPUT);  // set the SPI_CS pin as output
}


/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/

static StepperActuator *motor0, *motor1, *motor2;
#define ADCPIN A2
#define BIASPIN A0

static void rxLoop(unsigned long now);
static void processMsg(const char *msg, unsigned long now);

void loop(void)
{
    TF("::loop");
    
    unsigned long now = millis();

    rxLoop(now);

    if (!s_isOnline)
        return;

    assert(s_isOnline, "s_isOnline");
    assert(s_appChannel, "s_appChannel");
    assert(s_provisioner, "s_provisioner");
    assert(s_provisioner->hasConfig(), "s_provisioner->hasConfig()");
    assert(s_indicator, "s_indicator");
    
    switch (s_mainState) {
    case INIT_SENSORS: {
        TF("::loop; INIT_SENSORS");
        TRACE("Initializing sensors and actuators");
	s_mainState = ACQUIRE_TIMESTAMP;

	for (int i = 0; i < MAX_SENSORS; i++) {
	    s_sensors[i] = NULL;
	}
	for (int i = 0; i < Actuator::MAX_ACTUATORS; i++) {
	    s_actuators[i] = NULL;
	}

	s_timestamp = new Timestamp(CNF.getSSID(), CNF.getPSWD(),
				    CNF.getDbHost(), CNF.getDbPort(),
				    CNF.isSSL(), CNF.getDbUser(), CNF.getDbPswd());
	DL("Creating sensors and actuators");

	int actuatorIndex = 0, sensorIndex = 0;

	// register sensors and actuators
	SensorRateActuator *rate = new SensorRateActuator(CNF, "sample-rate", now);
	s_actuators[actuatorIndex++] = rate;

	s_sensors[sensorIndex++] = new HeartBeat(CNF, "heartbeat", *rate, *s_timestamp, now);
	s_sensors[sensorIndex++] = new TempSensor(CNF, "temp", *rate, *s_timestamp, now);
	s_sensors[sensorIndex++] = new HumidSensor(CNF, "humid", *rate, *s_timestamp, now);
	StepperActuator *motor0 = new StepperActuator(CNF, *rate, "motor0-target", now, 0x60, 1);
	s_sensors[sensorIndex++] = new StepperMonitor(CNF, "motor0", *rate, *s_timestamp, now, *motor0);
	s_actuators[actuatorIndex++] = motor0;
	StepperActuator *motor1 = new StepperActuator(CNF, *rate, "motor1-target", now, 0x60, 2);
	s_sensors[sensorIndex++] = new StepperMonitor(CNF, "motor1", *rate, *s_timestamp, now, *motor1);
	s_actuators[actuatorIndex++] = motor1;
	StepperActuator *motor2 = new StepperActuator(CNF, *rate, "motor2-target", now, 0x61, 2);
	s_sensors[sensorIndex++] = new StepperMonitor(CNF, "motor2", *rate, *s_timestamp, now, *motor2);
	s_actuators[actuatorIndex++] = motor2;
//	BeeCounter *beecnt = new BeeCounter(CNF, "beecnt", *rate, *s_timestamp, now, 
//					    BEECNT_PLOAD_PIN, BEECNT_CLOCK_PIN, BEECNT_DATA_PIN);
//	s_sensors[sensorIndex++] = beecnt;

//	s_listener = new Listener(ADCPIN, BIASPIN);

	s_currSensor = 0;

#if 0	
	ServoConfigActuator *servoConfig = new ServoConfigActuator("latch-config", now);
	s_actuators[4] = servoConfig;
	Latch *latch = new Latch("latch", *rate, now, LATCH_SERVO_PIN, *tempSensor, *servoConfig);
	s_sensors[6] = latch;
#endif
	
	PL("Sensors initialized;");
	TRACE("Sensors initialized;");

//	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(beecnt->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(motor0->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(motor1->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(motor2->getPulseGenConsumer());
//	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_20K(latch->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_20K_init();
	PL("PulseGenerators initialized;");
	
	// wait a bit for everything to settle
	delay(500);
    }
      break;

    case ACQUIRE_TIMESTAMP: {
        TF("::loop; AQUIRE_TIMESTAMP");
        assert(s_timestamp, "s_timestamp");
	
        if (!s_timestamp->haveTimestamp()) {
	    bool haveTimestamp = s_timestamp->loop(now, &sWifiMutex);
	    if (haveTimestamp) {
	        assert(s_timestamp->haveTimestamp(), "s_timestamp->haveTimestamp()");
		TRACE2("timestamp aquired at (ms): ", now);
		TRACE2("Timestamp: ", s_timestamp->markTimestamp());
		s_mainState = LOOP;
	    }
	}
    }
      break;
      
#ifdef bar      
    case CHECK_RX: {
        TF("::loop; CHECK_RX");
	HivePlatform::nonConstSingleton()->clearWDT();
	
	if (s_timestamp->haveTimestamp()) {
	    if (s_appChannel->loop(now, &sWifiMutex)) {
	        if (s_appChannel->haveMessage()) {
		    TRACE("Received a message from the App");
		    Str payload;
		    s_appChannel->consumePayload(&payload);
		    TRACE2("payload: ", payload.c_str());
		    processMsg(payload.c_str(), now);
#ifdef foo		    
		    if (payload.equals("motors")) {
		        motor0->processResult(now, "foo");
			motor1->processResult(now, "foo");
			motor2->processResult(now, "foo");
		    } else if (payload.equals("listen")) {
			bool stat = s_listener->record(10500, "/LISTEN.WAV", true);
			assert(stat, "Couldn't start recording");
			TRACE2("Listening... ", millis());

			bool done = false;
			while (!done) {
			  HivePlatform::nonConstSingleton()->clearWDT();
			  bool success = s_listener->loop(true);
			  if (s_listener->isDone()) {
			    if (s_listener->hasError()) {
			      TRACE2("Failed: ", s_listener->getErrmsg());
			    } else {
			      TRACE("Done: success! "); 
			    }
			    done = true;
			  }
			}
		    } else if (payload.equals("upload")) {
		        AudioUpload uploader(CNF, "audio-capture", "listen.wav", "audio/wav",
					     "/LISTEN.WAV", *s_rate, *s_timestamp, now);
			bool done = false;
			while (!done) {
			    HivePlatform::nonConstSingleton()->clearWDT();
			    if (uploader.isItTimeYet(millis())) {
			        //TRACE("calling uploader's loop");
			        bool callItBack = uploader.loop(millis(), &sWifiMutex);
				done = !callItBack;
				//TRACE2("uploader returned: ", (done ? "true":"false"));
			    }
			}
		    }
#endif
		}
	    }
	    
	    s_mainState = SENSOR_SAMPLE;
	} else {
	    bool haveTimestamp = s_timestamp->loop(now);
	    if (haveTimestamp) {
		TRACE2("timestamp aquired at (ms): ", now);
	        TRACE2("Timestamp: ", s_timestamp->markTimestamp());
	        s_mainState = LOOP;
	    }
	    
	    if (s_heartbeat->isItTimeYet(now)) 
	        s_heartbeat->loop(now, &sWifiMutex);
	}
    }
      break;
#endif
      
    case SENSOR_SAMPLE: {
        TF("::loop; SENSOR_SAMPLE");
	assert(s_timestamp->haveTimestamp(), "s_timestamp->haveTimestamp()");

	int sensorIndex = 0;
	while (s_sensors[sensorIndex]) {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    if (s_sensors[sensorIndex]->isItTimeYet(now))
	        s_sensors[sensorIndex]->loop(now, &sWifiMutex);

	    ++sensorIndex;
	}

	bool setChanged = true;
	while (setChanged) {
	    setChanged = false;
	    int l = Actuator::getNumActiveActuators();
	    HivePlatform::nonConstSingleton()->clearWDT();
	    for (int actuatorIndex = 0; !setChanged && actuatorIndex < l; actuatorIndex++) {
		bool callItBack = Actuator::getActiveActuator(actuatorIndex)->loop(now, &sWifiMutex);
		if (!callItBack) {
		    Actuator::deactivate(actuatorIndex);
		    setChanged = true;
		}
	    }
	}
    }
      break;
      
    default: {
        TF("::loop; default");
        ERR(Str("unknown value for s_mainState: ").append(s_mainState).c_str());
    }
    }
}


/* STATIC */
void processMsg(const char *msg, unsigned long now)
{
    TF("::processMsg");
    TRACE2("Msg to process: ", msg);

    int whichActuator = 0;
    if (s_timestamp && s_timestamp->haveTimestamp()) {
        while (s_actuators[whichActuator]) {
	    if (s_actuators[whichActuator]->isMyMsg(msg)) {
	        Actuator::activate(s_actuators[whichActuator]);
	    }
	    whichActuator++;
	}
    }

#ifdef foo    
    if (payload.equals("motors")) {
		        motor0->processResult(now, "foo");
			motor1->processResult(now, "foo");
			motor2->processResult(now, "foo");
		    } else if (payload.equals("listen")) {
			bool stat = s_listener->record(10500, "/LISTEN.WAV", true);
			assert(stat, "Couldn't start recording");
			TRACE2("Listening... ", millis());

			bool done = false;
			while (!done) {
			  HivePlatform::nonConstSingleton()->clearWDT();
			  bool success = s_listener->loop(true);
			  if (s_listener->isDone()) {
			    if (s_listener->hasError()) {
			      TRACE2("Failed: ", s_listener->getErrmsg());
			    } else {
			      TRACE("Done: success! "); 
			    }
			    done = true;
			  }
			}
		    } else if (payload.equals("upload")) {
		        AudioUpload uploader(CNF, "audio-capture", "listen.wav", "audio/wav",
					     "/LISTEN.WAV", *s_rate, *s_timestamp, now);
			bool done = false;
			while (!done) {
			    HivePlatform::nonConstSingleton()->clearWDT();
			    if (uploader.isItTimeYet(millis())) {
			        //TRACE("calling uploader's loop");
			        bool callItBack = uploader.loop(millis(), &sWifiMutex);
				done = !callItBack;
				//TRACE2("uploader returned: ", (done ? "true":"false"));
			    }
			}
		    }
#endif
}


/* STATIC */
void rxLoop(unsigned long now)
{
    TF("::rxLoop");

    assert(s_indicator, "s_indicator");
    
    HivePlatform::nonConstSingleton()->clearWDT();

    s_indicator->loop(now);
    
    if (s_appChannel == NULL) {
	if (s_provisioner->hasConfig() && s_provisioner->isStarted()) {
	    //TRACE("Has a valid config; stopping the provisioner");
	    s_provisioner->stop();
	    delay(500l);
	} else if (s_provisioner->hasConfig() && !s_provisioner->isStarted()) {
	    TRACE("Has a valid config and provisioner is stopped; starting AppChannel");
	    TRACE("If AppChannel cannot connect within 120s, will revert to Provisioning");
	    TRACE2("now: ", now);
	    s_indicator->setFlashMode(Indicator::TryingToConnect);
	    s_appChannel = new AppChannel(s_provisioner->getConfig(), now);
	} else if (!s_provisioner->isStarted()) {
	    TRACE("No valid config; starting the Provisioner");
	    s_indicator->setFlashMode(Indicator::Provisioning);
	    s_provisioner->start();
	}
    } else {
        if (s_appChannel->loop(now, &sWifiMutex)) {
	    if (s_appChannel->haveMessage()) {
	        TRACE("Received a message from the Mobile App");
		
		Str payload;
		s_appChannel->consumePayload(&payload);
		processMsg(payload.c_str(), now);
	    }
	}
	if (!s_isOnline && s_appChannel->isOnline()) {
	    if (!s_hasBeenOnline) {
	        PH2("Detected AppChannel online at: ", now);
	    }
	    s_indicator->setFlashMode(Indicator::Normal);
	    s_isOnline = true;
	    s_hasBeenOnline = true;
	} else if (s_isOnline && !s_appChannel->isOnline()) {
//	    PH2("Detected AppChannel offline at: ", now);
//	    s_indicator->setFlashMode(Indicator::Provisioning);
	    s_isOnline = false;
//	    s_provisioner->forcedStart();
//	    delete s_appChannel;
//	    s_appChannel = NULL;
	} else if (!s_isOnline &&
		   (now > s_appChannel->getOfflineTime() + 120*1000l) &&
		   sWifiMutex.isAvailable()) {
	    PH2("Detected AppChannel offline for more than 120s; initiating forced Provision mode at: ", now);
	    TRACE("Detected AppChannel offline; initiating forced Provision mode");
	    assert(sWifiMutex.isAvailable(), "sWifiMutex.isAvailable()");
	    s_isOnline = false;
	    s_indicator->setFlashMode(Indicator::Provisioning);
	    s_provisioner->forcedStart();
	    delete s_appChannel;
	    s_appChannel = NULL;
	}
    }

    if (s_provisioner->isStarted()) {
        if (!s_provisioner->loop(now, &sWifiMutex)) {
	    Str dump;
	    TRACE2("Using Config: ", CouchUtils::toString(s_provisioner->getConfig().getDoc(), &dump));
	}
    }
}


