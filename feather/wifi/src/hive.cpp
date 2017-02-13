#include <Arduino.h>

#define HEADLESS
#define NDEBUG

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
#include <beecnt.h>
#include <Heartbeat.h>
#include <Mutex.h>


#include <wifiutils.h>

#define CONFIG_FILENAME         "/CONFIG.CFG"

#define PROVISION               0
#define INIT_SENSORS            (PROVISION+1)
#define CHECK_RX                (INIT_SENSORS+1)
#define LOOP                    CHECK_RX
#define SENSOR_SAMPLE           (CHECK_RX+1)

#define BEECNT_PLOAD_PIN        10
#define BEECNT_CLOCK_PIN         9
#define BEECNT_DATA_PIN         11

static const char *ResetCause = "unknown";
static int s_mainState = PROVISION;

#define MAX_SENSORS 20
#define MAX_ACTUATORS 20
static Mutex sWifiMutex;
static Provision *s_provisioner = NULL;
static const HiveConfig *s_config = NULL;
static HeartBeat *s_heartbeat = NULL;
static int s_currSensor = 0;
static int s_currActuator = 0;
static Timestamp *s_timestamp = NULL;
static class Sensor *s_sensors[MAX_SENSORS];
static class Actuator *s_actuators[MAX_ACTUATORS];


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
    s_provisioner->startConfiguration();
    
    CHKPT("setup done");
    PL("Setup done...");
    
    pinMode(10, OUTPUT);  // set the SPI_CS pin as output
}


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
    case PROVISION: {
        TF("::loop; PROVISION");
	HivePlatform::nonConstSingleton()->clearWDT();
	if (s_provisioner->isItTimeYet(now)) {
	    if (!s_provisioner->loop(now, &sWifiMutex)) {
	        s_mainState = INIT_SENSORS;
		s_config = &s_provisioner->getConfig();  // DON'T delete s_provisioner!  Ever!

		Str dump;
		CouchUtils::toString(s_config->getDoc(), &dump);
		TRACE2("Using Config: ", dump.c_str());
	    }
	}
    }
      break;
      
    case INIT_SENSORS: {
        TF("::loop; INIT_SENSORS");
        TRACE("Initializing sensors and actuators");
	s_mainState = LOOP;

	for (int i = 0; i < MAX_SENSORS; i++) {
	    s_sensors[i] = NULL;
	}
	for (int i = 0; i < MAX_ACTUATORS; i++) {
	    s_actuators[i] = NULL;
	}
	
	s_timestamp = new Timestamp(s_config->getSSID(), s_config->getPSWD(),
				    s_config->getDbHost(), s_config->getDbPort(),
				    s_config->isSSL(), s_config->getDbCredentials());
	DL("Creating sensors and actuators");
	
	s_currSensor = 0;
	s_currActuator = 0;

	// register sensors and actuators
	SensorRateActuator *rate = new SensorRateActuator(*s_config, "sample-rate", now);
	s_actuators[s_currActuator++] = rate;

	s_sensors[s_currSensor++] = s_heartbeat = new HeartBeat(*s_config, "heartbeat",
								*rate, *s_timestamp, now);
	s_sensors[s_currSensor++] = new TempSensor(*s_config, "temp", *rate, *s_timestamp, now);
	s_sensors[s_currSensor++] = new HumidSensor(*s_config, "humid", *rate, *s_timestamp, now);
	StepperActuator *motor0 = new StepperActuator(*s_config, *rate, "motor0-target", now, 0x60, 1);
	s_sensors[s_currSensor++] = new StepperMonitor(*s_config, "motor0", *rate, *s_timestamp, now, *motor0);
	s_actuators[s_currActuator++] = motor0;
	StepperActuator *motor1 = new StepperActuator(*s_config, *rate, "motor1-target", now, 0x60, 2);
	s_sensors[s_currSensor++] = new StepperMonitor(*s_config, "motor1", *rate, *s_timestamp, now, *motor1);
	s_actuators[s_currActuator++] = motor1;
	StepperActuator *motor2 = new StepperActuator(*s_config, *rate, "motor2-target", now, 0x61, 2);
	s_sensors[s_currSensor++] = new StepperMonitor(*s_config, "motor2", *rate, *s_timestamp, now, *motor2);
	s_actuators[s_currActuator++] = motor2;
//	BeeCounter *beecnt = new BeeCounter(*s_config, "beecnt", *rate, *s_timestamp, now, 
//					    BEECNT_PLOAD_PIN, BEECNT_CLOCK_PIN, BEECNT_DATA_PIN);
//	s_sensors[s_currSensor++] = beecnt;
	
	s_currSensor = 0;
	s_currActuator = 0;

#if 0	
	ServoConfigActuator *servoConfig = new ServoConfigActuator("latch-config", now);
	s_actuators[4] = servoConfig;
	Latch *latch = new Latch("latch", *rate, now, LATCH_SERVO_PIN, *tempSensor, *servoConfig);
	s_sensors[6] = latch;

	s_bleStream = new BleStream(&ble, s_timestamp, s_sensors, s_actuators);
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
	
	// wait a bit for comms to settle
	delay(500);
    }
      break;
      
    case CHECK_RX: {
        TF("::loop; CHECK_RX");
	HivePlatform::nonConstSingleton()->clearWDT();
	
	if (s_timestamp->haveTimestamp()) {
	    s_mainState = SENSOR_SAMPLE;
	} else {
	    bool haveTimestamp = s_timestamp->loop(now);
	    if (haveTimestamp) {
	        TRACE2("Timestamp: ", s_timestamp->markTimestamp());
	        s_mainState = LOOP;
	    }
	    
	    if (s_heartbeat->isItTimeYet(now)) 
	        s_heartbeat->loop(now, &sWifiMutex);
	}
    }
      break;

    case SENSOR_SAMPLE: {
        TF("::loop; SENSOR_SAMPLE");
        if (s_sensors[s_currSensor]->isItTimeYet(now)) 
	    s_sensors[s_currSensor]->loop(now, &sWifiMutex);

	if (s_sensors[++s_currSensor] == NULL)
	    s_currSensor = 0;
	
        if (s_actuators[s_currActuator]->isItTimeYet(now)) 
	    s_actuators[s_currActuator]->loop(now, &sWifiMutex);

	if (s_actuators[++s_currActuator] == NULL)
	    s_currActuator = 0;
	
	s_mainState = LOOP;
    }
      break;
      
    default: {
        TF("::loop; default");
        ERR(Str("unknown value for s_mainState: ").append(s_mainState).c_str());
    }
    }
}
