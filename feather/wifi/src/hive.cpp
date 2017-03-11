#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>

#include <version_id.h>

#include <Provision.h>
#include <hiveconfig.h>
#include <ConfigUploader.h>
#include <ConfigPersister.h>
#include <AccessPointActuator.h>
#include <CrashUpload.h>
#include <SensorRateActuator.h>
#include <ServoConfigActuator.h>
#include <latch.h>
#include <TempSensor.h>
#include <HumidSensor.h>
#include <StepperMonitor.h>
#include <StepperActuator.h>
#include <beecnt.h>
#include <Indicator.h>
#include <Heartbeat.h>
#include <AppChannel.h>
#include <http_op.h>
#include <ListenSensor.h>
#include <ListenActuator.h>
#include <Mutex.h>

#include <strbuf.h>

#include <wifiutils.h>

#define CONFIG_FILENAME         "/CONFIG.CFG"

#define INIT_SENSORS            0
#define SENSOR_SAMPLE           (INIT_SENSORS+1)
#define LOOP                    SENSOR_SAMPLE

#define BEECNT_PLOAD_PIN        10
#define BEECNT_CLOCK_PIN         9
#define BEECNT_DATA_PIN         11

#define LATCH_SERVO_PIN         12

static const char *ResetCause = "unknown";
static int s_mainState = INIT_SENSORS;
static bool s_isOnline = false;
static bool s_hasBeenOnline = false;

#define MAX_SENSORS 20
static Mutex sWifiMutex, sSdMutex;
static Provision *s_provisioner = NULL;
static ConfigUploader *s_configUploader = NULL;
static ConfigPersister *s_configPersister = NULL;
static Indicator *s_indicator = NULL;
static AppChannel *s_appChannel = NULL;
static int s_currSensor = 0;
static class Sensor *s_sensors[MAX_SENSORS];
static class Actuator *s_actuators[Actuator::MAX_ACTUATORS];
static BeeCounter *s_beecnt = NULL;

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
    TF("HiveConfigPersister::onUpdate");
    if (s_configUploader && s_provisioner && (&c == &CNF)) {
        s_configPersister->persist();
        s_configUploader->upload();
    }

    HiveConfigFunctor::onUpdate(c);
}


static HttpOp::YieldHandler sPrevYieldHandler = NULL;
void MyYieldHandler()
{
    if (s_beecnt != NULL)
        s_beecnt->sample(millis());

    if (sPrevYieldHandler != NULL)
        sPrevYieldHandler();
}

  
/**************************************************************************/
/*!
    @brief  Sets up the HW (this function is called automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
    TF("::setup");
    
    // capture the reason for previous reset so I can inform the app
    ResetCause = HivePlatform::singleton()->getResetCause(); 
    
    delay(500);

#ifndef HEADLESS
    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
#endif

    PH2("ResetCause: ", ResetCause);

    pinMode(5, OUTPUT);
    
    //pinMode(5, OUTPUT);         // for debugging: used to indicate ISR invocations for motor drivers
    
    delay(500);
    
    PL("Hive Controller debug console");
    PL("---------------------------------------");

    s_provisioner = new Provision(ResetCause, VERSION, CONFIG_FILENAME, millis(), &sWifiMutex);
    
    // setup callback to persist and upload all hive config changes (including whatever
    // one gets chosen by provisioner)
    HiveConfigPersister *persister = new HiveConfigPersister(CONFIG_FILENAME);
    persister->setPrevUpdater(CNF.onUpdate(persister));

    sPrevYieldHandler = HttpOp::registerYieldHandler(MyYieldHandler);
    
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

    now = millis();
    
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

	// register sensors and actuators
	SensorRateActuator *rate = new SensorRateActuator(&CNF, "sample-rate", now);
	s_actuators[actuatorIndex++] = rate;

	s_configUploader = new ConfigUploader(CNF, *rate, *s_appChannel, now, &sWifiMutex);
	s_configUploader->upload(); // to force an initial upload on every run
	s_sensors[sensorIndex++] = s_configUploader;
	
	s_sensors[sensorIndex++] = new HeartBeat(CNF, "heartbeat", *rate, *s_appChannel, now, &sWifiMutex);
	TempSensor *tempSensor = new TempSensor(CNF, "temp", *rate, *s_appChannel, now, &sWifiMutex);
	s_sensors[sensorIndex++] = tempSensor;
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
	BeeCounter *beecnt = s_beecnt = new BeeCounter(CNF, "beecnt", *rate, *s_appChannel, now, 
					      BEECNT_PLOAD_PIN, BEECNT_CLOCK_PIN, BEECNT_DATA_PIN,
					      &sWifiMutex);
	s_sensors[sensorIndex++] = beecnt;

	ListenSensor *listener = new ListenSensor(CNF, "listener", *rate, *s_appChannel, now,
						  ADCPIN, BIASPIN, &sWifiMutex, &sSdMutex);
	ListenActuator *listenControl = new ListenActuator(*listener, "listen-ctrl", now);
	s_sensors[sensorIndex++] = listener;
	s_actuators[actuatorIndex++] = listenControl;

	s_sensors[sensorIndex++] = new CrashUpload(CNF, "crash-report", "report.txt", "text/plain",
						   *rate, *s_appChannel, now, &sWifiMutex, &sSdMutex);
	s_currSensor = 0;

	ServoConfigActuator *servoConfig = new ServoConfigActuator(&CNF, "latch-config", now);
	s_actuators[actuatorIndex++] = servoConfig;
	Latch *latch = new Latch(CNF, "latch", *rate, *s_appChannel, now,
				 LATCH_SERVO_PIN, *tempSensor, *servoConfig, &sWifiMutex);
	s_sensors[sensorIndex++] = latch;

	s_configPersister = new ConfigPersister(CNF, *rate, *s_appChannel,
						CONFIG_FILENAME, now, &sSdMutex);
	s_sensors[sensorIndex++] = s_configPersister;

	s_actuators[actuatorIndex++] = new AccessPointActuator(&CNF, "access-point", now);
	
	PH2(sensorIndex, " Sensors initialized;");
	PH2(actuatorIndex, " Actuators initialized;");

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_22K(latch->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_22K_init();
	PL("PulseGenerators initialized;");
	
	// wait a bit for everything to settle
	delay(500);
    }
      break;

    case SENSOR_SAMPLE: {
        TF("::loop; SENSOR_SAMPLE");
	assert(s_appChannel->haveTimestamp(), "s_appChannel->haveTimestamp()");

	int sensorIndex = 0;
	while (s_sensors[sensorIndex]) {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    unsigned long mark = now;
	    if (s_sensors[sensorIndex]->isItTimeYet(now)) {
	        //TRACE2("invoking loop for sensor ", sensorIndex);
	        s_sensors[sensorIndex]->loop(now);
		now = millis();
		if (now - mark > 20) {
		    TRACE4("Sensor ", sensorIndex, " took longer than expected; delta: ", (now-mark));
		}
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
    
    s_beecnt->sample(now);
	  
    if (s_appChannel == NULL) {
	if (s_provisioner->hasConfig() && s_provisioner->isStarted()) {
	    //TRACE("Has a valid config; stopping the provisioner");
	    s_provisioner->stop();
	    delay(500l);
	} else if (s_provisioner->hasConfig() && !s_provisioner->isStarted()) {
	    TRACE("Has a valid config and provisioner is stopped; starting AppChannel");
	    TRACE("If AppChannel cannot connect within 120s, will revert to Provisioning");
	    s_indicator->setFlashMode(Indicator::TryingToConnect);
	    s_appChannel = new AppChannel(CNF, now, &sWifiMutex, &sSdMutex);
	    PH2("s_isOnline: ", (s_isOnline ? "true" : "false"));
	    PH2("AppChannel says it went offline at: ", s_appChannel->getOfflineTime());
	} else if (!s_provisioner->isStarted()) {
	    TRACE("No valid config; starting the Provisioner");
	    s_indicator->setFlashMode(Indicator::Provisioning);
	    s_provisioner->start(now);
	} else if (now > s_provisioner->getStartTime() + 120*1000l) {
	    TRACE("Stopping the provisioner to try to connect with existing config");
	    TRACE("If AppChannel cannot connect within 120s, will revert to Provisioning");
	    s_provisioner->stop();
	    delay(500l);
	    if (s_provisioner->hasConfig()) {
	        s_indicator->setFlashMode(Indicator::TryingToConnect);
		s_appChannel = new AppChannel(CNF, now, &sWifiMutex, &sSdMutex);
		PH2("s_isOnline: ", (s_isOnline ? "true" : "false"));
		PH2("AppChannel says it went offline at: ", s_appChannel->getOfflineTime());
	    }
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
	    s_indicator->setFlashMode(Indicator::TryingToConnect);
	    PH2("Detected AppChannel offline at: ", now);
	    PH2("AppChannel says it went offline at: ", s_appChannel->getOfflineTime());
	} else {
	    if (!s_isOnline &&
		(now > s_appChannel->getOfflineTime() + 120*1000l) &&
		sWifiMutex.isAvailable()) {
	        assert(sWifiMutex.isAvailable(), "sWifiMutex.isAvailable()");
		PH2("Detected AppChannel offline for more than 120s; initiating forced Provision mode at: ", now);
		TRACE("Detected AppChannel offline; initiating forced Provision mode");
		s_isOnline = false;
		s_indicator->setFlashMode(Indicator::Provisioning);
		s_provisioner->forcedStart(now);
		delete s_appChannel;
		s_appChannel = NULL;
		s_indicator->setFlashMode(Indicator::Provisioning);
	    }
	}
    }

    if (s_provisioner->isStarted()) {
        if (!s_provisioner->loop(now)) {
	    StrBuf dump;
	    TRACE2("Using Config: ", CouchUtils::toString(CNF.getDoc(), &dump));
	}
    }
}


