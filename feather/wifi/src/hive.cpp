#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>

#include <version_id.h>

#include <Provision.h>
#include <hiveconfig.h>
#include <ConfigUploader.h>
#include <docwriter.h>
#include <SensorRateActuator.h>
#include <TempSensor.h>
#include <HumidSensor.h>
#include <StepperMonitor.h>
#include <StepperActuator.h>
#include <AudioUpload.h>
#include <beecnt.h>
#include <Indicator.h>
#include <Heartbeat.h>
#include <AppChannel.h>
#include <ListenSensor.h>
#include <ListenActuator.h>
#include <Mutex.h>


#include <wifiutils.h>

#define CONFIG_FILENAME         "/CONFIG.CFG"

#define INIT_SENSORS            0
#define SENSOR_SAMPLE           (INIT_SENSORS+1)
#define LOOP                    SENSOR_SAMPLE

#define BEECNT_PLOAD_PIN        10
#define BEECNT_CLOCK_PIN         9
#define BEECNT_DATA_PIN         11

static const char *ResetCause = "unknown";
static int s_mainState = INIT_SENSORS;
static bool s_isOnline = false;
static bool s_hasBeenOnline = false;

#define MAX_SENSORS 20
static Mutex sWifiMutex, sSdMutex;
static Provision *s_provisioner = NULL;
static ConfigUploader *s_configUploader = NULL;
static Indicator *s_indicator = NULL;
static AppChannel *s_appChannel = NULL;
static int s_currSensor = 0;
static class Sensor *s_sensors[MAX_SENSORS];
static class Actuator *s_actuators[Actuator::MAX_ACTUATORS];

#define CNF s_provisioner->getConfig()



class HiveConfigFunctor : public HiveConfig::UpdateFunctor {
public:
    HiveConfig::UpdateFunctor *mPrev;
  
    HiveConfigFunctor() : mPrev(NULL) {}
    ~HiveConfigFunctor() {delete mPrev;}
  
    void setPrevUpdater(HiveConfig::UpdateFunctor *prev) {mPrev = prev;}
    void onUpdate(const HiveConfig &c) {if (mPrev) mPrev->onUpdate(c);}
};

class HiveConfigPersister : public HiveConfigFunctor {
public:
    const char *mFilename;
  
    HiveConfigPersister(const char *filename) : mFilename(filename) {}
  
    void onUpdate(const HiveConfig &c);
};

void HiveConfigPersister::onUpdate(const HiveConfig &c) {
    if (s_provisioner && (&c == &s_provisioner->getConfig())) {
        DocWriter writer(mFilename, c.getDoc(), &sSdMutex);
	while (writer.loop()) {}
	assert(writer.isDone(), "Couldn't write updated config to file");
	assert(!writer.hasError(), "Couldn't write updated config to file");
    }

    HiveConfigFunctor::onUpdate(c);
}
  
class HiveConfigUploader : public HiveConfigFunctor {
public:
    void onUpdate(const HiveConfig &c);
};

void HiveConfigUploader::onUpdate(const HiveConfig &c) {
    if (s_configUploader && s_provisioner && (&c == &s_provisioner->getConfig())) {
        s_configUploader->upload();
    }

    HiveConfigFunctor::onUpdate(c);
}
  

  
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
  
    PH2("ResetCause: ", ResetCause);
    
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
  
    s_provisioner = new Provision(ResetCause, VERSION, CONFIG_FILENAME, millis(), &sWifiMutex);
    
    // setup callback to upload hive config changes (including whatever one gets chosen by provisioner)
    HiveConfigUploader *uploader = new HiveConfigUploader();
    uploader->setPrevUpdater(CNF.onUpdate(uploader));
	
    s_indicator = new Indicator(millis());
    s_indicator->setFlashMode(Indicator::TryingToConnect);
    
    PH("Setup done...");

    pinMode(10, OUTPUT);  // set the SPI_CS pin as output
}


/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/

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
        PH("Initializing sensors and actuators");
	s_mainState = SENSOR_SAMPLE;

	for (int i = 0; i < MAX_SENSORS; i++) {
	    s_sensors[i] = NULL;
	}
	for (int i = 0; i < Actuator::MAX_ACTUATORS; i++) {
	    s_actuators[i] = NULL;
	}

	int actuatorIndex = 0, sensorIndex = 0;

	// setup callback to persist any future hive config changes (don't set this up earlier, or you'll
	// save the config to file immediately after reading it from the same file!)
	HiveConfigPersister *persister = new HiveConfigPersister(CONFIG_FILENAME);
	persister->setPrevUpdater(CNF.onUpdate(persister));

	// register sensors and actuators
	SensorRateActuator *rate = new SensorRateActuator(&s_provisioner->getConfig(), "sample-rate", now);
	s_actuators[actuatorIndex++] = rate;

	s_configUploader = new ConfigUploader(s_provisioner->getConfig(), *rate, *s_appChannel, now, &sWifiMutex);
	s_sensors[sensorIndex++] = s_configUploader;
	
	s_sensors[sensorIndex++] = new HeartBeat(CNF, "heartbeat", *rate, *s_appChannel, now, &sWifiMutex);
	s_sensors[sensorIndex++] = new TempSensor(CNF, "temp", *rate, *s_appChannel, now, &sWifiMutex);
	s_sensors[sensorIndex++] = new HumidSensor(CNF, "humid", *rate, *s_appChannel, now, &sWifiMutex);
	StepperActuator *motor0 = new StepperActuator(CNF, *rate, "motor0-target", now, 0x60, 1);
	s_sensors[sensorIndex++] = new StepperMonitor(CNF, "motor0", *rate, *s_appChannel, now, *motor0,
						      &sWifiMutex);
	s_actuators[actuatorIndex++] = motor0;
	StepperActuator *motor1 = new StepperActuator(CNF, *rate, "motor1-target", now, 0x60, 2);
	s_sensors[sensorIndex++] = new StepperMonitor(CNF, "motor1", *rate, *s_appChannel, now, *motor1,
						      &sWifiMutex);
	s_actuators[actuatorIndex++] = motor1;
	StepperActuator *motor2 = new StepperActuator(CNF, *rate, "motor2-target", now, 0x61, 2);
	s_sensors[sensorIndex++] = new StepperMonitor(CNF, "motor2", *rate, *s_appChannel, now, *motor2,
						      &sWifiMutex);
	s_actuators[actuatorIndex++] = motor2;
//	BeeCounter *beecnt = new BeeCounter(CNF, "beecnt", *rate, *s_appChannel, now, 
//					    BEECNT_PLOAD_PIN, BEECNT_CLOCK_PIN, BEECNT_DATA_PIN,
//					    &sWifiMutex);
//	s_sensors[sensorIndex++] = beecnt;

	ListenSensor *listener = new ListenSensor(CNF, "listener", *rate, *s_appChannel, now,
						  ADCPIN, BIASPIN, &sWifiMutex, &sSdMutex);
	ListenActuator *listenControl = new ListenActuator(*listener, "listen-ctrl", now);
	s_sensors[sensorIndex++] = listener;
	s_actuators[actuatorIndex++] = listenControl;

	s_currSensor = 0;

#if 0	
	ServoConfigActuator *servoConfig = new ServoConfigActuator("latch-config", now);
	s_actuators[4] = servoConfig;
	Latch *latch = new Latch("latch", *rate, now, LATCH_SERVO_PIN, *tempSensor, *servoConfig);
	s_sensors[6] = latch;
#endif
	
	PL("Sensors initialized;");
	TRACE("Sensors initialized;");

//	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_11K(beecnt->getPulseGenConsumer());
//	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_22K(latch->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_22K_init();
	PL("PulseGenerators initialized;");
	
	// wait a bit for everything to settle
	delay(500);
    }
      break;

#ifdef foo		    
		    if (payload.equals("motors")) {
		    } else if (payload.equals("listen")) {
		    } else if (payload.equals("upload")) {
		        AudioUpload uploader(CNF, "audio-capture", "listen.wav", "audio/wav",
					     "/LISTEN.WAV", *s_rate, *s_timestamp, now);
			bool done = false;
			while (!done) {
			    HivePlatform::nonConstSingleton()->clearWDT();
			    if (uploader.isItTimeYet(millis())) {
			        //TRACE("calling uploader's loop");
			        bool callItBack = uploader.loop(millis());
				done = !callItBack;
				//TRACE2("uploader returned: ", (done ? "true":"false"));
			    }
			}
		    }
#endif
      
    case SENSOR_SAMPLE: {
        TF("::loop; SENSOR_SAMPLE");
	assert(s_appChannel->haveTimestamp(), "s_appChannel->haveTimestamp()");

	int sensorIndex = 0;
	while (s_sensors[sensorIndex]) {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    if (s_sensors[sensorIndex]->isItTimeYet(now)) {
	        s_sensors[sensorIndex]->loop(now);
		rxLoop(now); // consider rx after every operation to ensure responsiveness from app
	    }

	    ++sensorIndex;
	}

	bool setChanged = true;
	while (setChanged) {
	    setChanged = false;
	    int l = Actuator::getNumActiveActuators();
	    HivePlatform::nonConstSingleton()->clearWDT();
	    for (int actuatorIndex = 0; !setChanged && actuatorIndex < l; actuatorIndex++) {
		bool callItBack = Actuator::getActiveActuator(actuatorIndex)->loop(now);
		if (!callItBack) {
		    TRACE2("Deactivating actuator: ", Actuator::getActiveActuator(actuatorIndex)->getName());
		    Actuator::deactivate(actuatorIndex);
		    setChanged = true;
		}
	    }
	}
    }
      break;
      
    default: {
        TF("::loop; default");
	assert2(false, "Unknown value for s_mainState: ", s_mainState);
    }
    }
}


/* STATIC */
void processMsg(const char *msg, unsigned long now)
{
    TF("::processMsg");
    PH2("Msg to process: ", msg);

    assert(s_appChannel, "s_appChannel");
    assert(s_appChannel->haveTimestamp(), "s_appChannel->haveTimestamp()");
    
    int whichActuator = 0;
    bool foundConsumer = false;
    while (s_actuators[whichActuator]) {
        HivePlatform::nonConstSingleton()->clearWDT();
        if (s_actuators[whichActuator]->isMyMsg(msg)) {
	    TRACE2("Activating actuator: ", s_actuators[whichActuator]->getName());
	    foundConsumer = true;
	    Actuator::activate(s_actuators[whichActuator]);
	    s_actuators[whichActuator]->processMsg(now, msg);
	}
	whichActuator++;
    }

    if (!foundConsumer) {
        PH2("WARNING: received a message from the App that has not been claimed: ", msg);
    }
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
	    s_appChannel = new AppChannel(s_provisioner->getConfig(), now, &sWifiMutex, &sSdMutex);
	} else if (!s_provisioner->isStarted()) {
	    TRACE("No valid config; starting the Provisioner");
	    s_indicator->setFlashMode(Indicator::Provisioning);
	    s_provisioner->start();
	}
    } else {
        if (s_appChannel->loop(now)) {
	    if (s_appChannel->haveMessage() && sSdMutex.isAvailable() && sSdMutex.own(s_appChannel)) {
	        assert(sSdMutex.whoOwns() == s_appChannel, "sSdMutex.whoOwns() == s_appChannel");
		Str payload;
		s_appChannel->consumePayload(&payload, &sSdMutex);
		assert(sSdMutex.whoOwns() == s_appChannel, "sSdMutex.whoOwns() == s_appChannel");
		processMsg(payload.c_str(), now);
		sSdMutex.release(s_appChannel);
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
	    s_isOnline = false;
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
        if (!s_provisioner->loop(now)) {
	    Str dump;
	    TRACE2("Using Config: ", CouchUtils::toString(s_provisioner->getConfig().getDoc(), &dump));
	}
    }
}


