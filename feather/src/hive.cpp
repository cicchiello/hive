#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BluefruitLE_SPI.h"

#include "BluefruitConfig.h"

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
#define assert(c,msg) if (!(c)) {PL("ASSERT"); WDT_TRACE(msg); while(1);}
#else
#define assert(c,msg) do {} while(0);
#endif


#include <hive_platform.h>
#define TRACE(msg) HivePlatform::singleton()->trace(msg)
#define ERR(msg) HivePlatform::singleton()->error(msg)


#include <strutils.h>
#include <Parse.h>
#include <cloudpipe.h>
#include <str.h>
#include <txqueue.h>
#include <Timestamp.h>

#include <SensorRateActuator.h>
#include <CpuTempSensor.h>
#include <TempSensor.h>
#include <HumidSensor.h>
#include <StepperMonitor.h>
#include <StepperActuator.h>
#include <beecnt.h>


/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


#define CONNECTED_LED           13

#define INIT 0
#define CHECK_BLE_RX            (INIT+1)
#define LOOP                    CHECK_BLE_RX
#define SENSOR_SAMPLE           (CHECK_BLE_RX+1)
#define QUEUE_TIMESTAMP_REQUEST (SENSOR_SAMPLE+1)
#define ATTEMPT_SENSOR_POST     (QUEUE_TIMESTAMP_REQUEST+1)
#define VISIT_ACTUATOR          (ATTEMPT_SENSOR_POST+1)
#define ATTEMPT_TIMESTAMP_POST  (VISIT_ACTUATOR+1)
#define TEST_CONNECTION         (ATTEMPT_TIMESTAMP_POST+1)
#define TEST_DISCONNECT         (TEST_CONNECTION+1)


static const char *ResetCause = "unknown";
static int s_mainState = INIT;

#define BLE_INPUT_POLL_RATE 1000
#define MAX_SENSORS 20
#define MAX_ACTUATORS 20
static int s_sensorSampleState = 0;
static int s_currSensor = -1;
static int s_currActuator = -1;
static bool s_connected = false;
static unsigned long s_nextConnectCheckTime = 0;
static Str *s_rxLine = NULL;
static unsigned long s_nextDisconnectTime = 2*60*1000;
static Timestamp *s_timestamp = 0;
static Sensor *s_sensors[MAX_SENSORS];
static Actuator *s_actuators[MAX_ACTUATORS];


static bool setIsBLEConnected(bool v);
static void handleBLEInput(Adafruit_BluefruitLE_SPI &ble);





/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
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
  
    pinMode(CONNECTED_LED, OUTPUT);  // indicates BLE connection

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

    /* Initialise the module */
    P("Initialising the Bluefruit LE module: ");

#ifndef HEADLESS
    if ( !ble.begin(VERBOSE_MODE) )
    {
        ERR("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?");
    }
#else
    if ( !ble.begin(false) )
    {
        ERR("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?");
    }
#endif

    delay(1500); // a little extra delay after the BLE reboot
    TRACE("ble initialized");
  
    PL("OK!");

    if ( FACTORYRESET_ENABLE )
    {
        /* Perform a factory reset to make sure everything is in a known state */
        PL("Performing a factory reset: ");
	if ( ! ble.factoryReset() ){
	    ERR("Couldn't factory reset");
	}
    }
    delay(1500); // a little extra delay after the BLE reset

    /* Disable command echo from Bluefruit */
    ble.echo(false);

    PL("Requesting Bluefruit info:");
    /* Print Bluefruit information */
#ifndef HEADLESS
    ble.info();
#endif

    PL();
    PL("Awaiting connection");
    
    ble.verbose(false);  // debug info is a little annoying after this point!

    setIsBLEConnected(false);
  
    HivePlatform::nonConstSingleton()->startWDT();
  
    /* Wait for connection */
    while (! ble.isConnected()) {
        HivePlatform::nonConstSingleton()->clearWDT();
	delay(500);
    }

    setIsBLEConnected(true);
    TRACE("Connected");
    PL("Connected");
    PL();
  
    // LED Activity command is only supported from 0.6.6
    if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
    {
        // Change Mode LED Activity
        PL("******************************");
	PL("Change LED activity to " MODE_LED_BEHAVIOUR);
	ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
	PL("******************************");
    }
}

#define BEECNT_PLOAD_PIN        10
#define BEECNT_CLOCK_PIN         9
#define BEECNT_DATA_PIN         11

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/

void loop(void)
{
    unsigned long now = millis();
    switch (s_mainState) {
    case INIT: {
        TRACE("Initializing sensors and actuators");
	s_mainState = LOOP;
	CloudPipe::nonConstSingleton().initMacAddress(ble);
	s_timestamp = new Timestamp(ResetCause);

	for (int i = 0; i < MAX_SENSORS; i++) {
	    s_sensors[i] = NULL;
	}
	for (int i = 0; i < MAX_ACTUATORS; i++) {
	    s_actuators[i] = NULL;
	}
	
	s_currSensor = 0;
	s_currActuator = 0;

	DL("Creating sensors and actuators");
	
	// register sensors
	SensorRateActuator *rate = new SensorRateActuator("sample-rate", now);
	s_sensors[0] = new CpuTempSensor("cputemp", *rate, now, ble);
	s_sensors[1] = new TempSensor("temp", *rate, now);
	s_sensors[2] = new HumidSensor("humid", *rate, now);
	StepperActuator *motor0 = new StepperActuator("motor0", 0x60, 1, now);
	s_sensors[3] = new StepperMonitor(*motor0, *rate, now);
	s_actuators[0] = motor0;
	StepperActuator *motor1 = new StepperActuator("motor1", 0x60, 2, now);
	s_sensors[4] = new StepperMonitor(*motor1, *rate, now);
	s_actuators[1] = motor1;
	StepperActuator *motor2 = new StepperActuator("motor2", 0x61, 2, now);
	s_sensors[5] = new StepperMonitor(*motor2, *rate, now);
	s_actuators[2] = motor2;
	s_actuators[3] = rate;
	BeeCounter *beecnt = new BeeCounter("beecnt", *rate, now,
					    BEECNT_PLOAD_PIN, BEECNT_CLOCK_PIN, BEECNT_DATA_PIN);
	s_sensors[6] = beecnt;
	
	PL("Sensors initialized;");
	TRACE("Sensors initialized;");

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(beecnt->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(motor0->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(motor1->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_10K(motor2->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_10K_init();
	PL("PulseGenerators initialized;");
	
	// wait a bit for comms to settle
	delay(500);
    }
      break;
      
    case CHECK_BLE_RX: {
        TRACE("checking for BLE rx");
	HivePlatform::nonConstSingleton()->clearWDT();
	
        // Check for incoming transmission from Bluefruit
        handleBLEInput(ble);

	if (s_timestamp->haveTimestamp())
	    s_mainState = SENSOR_SAMPLE;
	else if (s_timestamp->haveRequestedTimestamp())
	    s_mainState = ATTEMPT_TIMESTAMP_POST;
	else
	    s_mainState = QUEUE_TIMESTAMP_REQUEST;

    }
    break;

    case QUEUE_TIMESTAMP_REQUEST: {
        if (now > 10000) {
	    DL("timestamp enqueueRequest");
	    s_timestamp->enqueueRequest();
	}
	s_mainState = LOOP;
    }
    break;
    
    case ATTEMPT_TIMESTAMP_POST: {
        TRACE("Attempting timestamp post");
        s_timestamp->attemptPost(ble);
	s_mainState = LOOP;
    }
      break;

    case SENSOR_SAMPLE: {
        TRACE("Sensor sample state");
	
	// see if it's time to sample
	if (s_sensors[s_currSensor]->isItTimeYet(now)) {
	    TRACE("starting sampling sensor");
	    s_sensors[s_currSensor]->scheduleNextSample(now);

	    Str sensorValueStr;
	    if (s_sensors[s_currSensor]->sensorSample(&sensorValueStr)) {
	        Str timestampStr;
		s_timestamp->toString(now, &timestampStr);
	    
		s_sensors[s_currSensor]->enqueueRequest(sensorValueStr.c_str(),
							timestampStr.c_str());
		TRACE("done sampling sensor");
		
		s_mainState = ATTEMPT_SENSOR_POST;
		s_sensorSampleState = 0;
	    } else {
	        s_mainState = VISIT_ACTUATOR;
		s_sensorSampleState = 1;
	    }
	} else {
	    s_mainState = ATTEMPT_SENSOR_POST;
	}
    }
      break;

    case ATTEMPT_SENSOR_POST: {
        TRACE("Attempting sensor post");
        s_sensors[s_currSensor]->attemptPost(ble);
	
	s_currSensor++;
	if (s_sensors[s_currSensor] == NULL) {
	    s_currSensor = 0;
	}
	
	s_mainState = VISIT_ACTUATOR;
    }
      break;
      
    case VISIT_ACTUATOR: {
        // see if it's time to sample
        TRACE("visiting actuator");
        if (s_actuators[s_currActuator]->isItTimeYet(now)) {
	    s_actuators[s_currActuator]->scheduleNextAction(now);

	    s_actuators[s_currActuator]->act(ble);
	    
	    s_currActuator++;
	    if (s_actuators[s_currActuator] == NULL) {
	        s_currActuator = 0;
	    }
	
	    TRACE("done visiting actuator");
	}

	s_mainState = TEST_CONNECTION;
    }
      break;
      
    case TEST_CONNECTION: {
	TRACE("testing connection");
	if (now > s_nextConnectCheckTime) {
	    if (s_connected) {
	        if (!ble.isConnected()) {
		    PL("Discovered disconnect; checking again in 5s");
		    setIsBLEConnected(false);
		}
	    } else {
	        // see if reconnection has happened
	        if (ble.isConnected()) {
		    PL("Reconnected!");
		    HivePlatform::nonConstSingleton()->clearWDT();
		    delay(500);
		    setIsBLEConnected(true);
		} else {
		    PL("Not connected; Scheduling a connection check in 5s");
		}
	    }
	    s_nextConnectCheckTime = millis() + 5*1000;
	}
	s_mainState = LOOP;
    }
      break;
      
    case TEST_DISCONNECT: {
        if (now > s_nextDisconnectTime) {

	    if (s_connected) {
	        if (ble.isConnected()) {
		    PL("Disconnecting...");
		    ble.disconnect();
		    setIsBLEConnected(false);
		
		    // see if reconnection has happened immediately
		    HivePlatform::nonConstSingleton()->clearWDT();
		    delay(500);
		    if (ble.isConnected()) {
		        s_nextDisconnectTime = millis() + 70*1000;
			PL("Reconnected!");
			HivePlatform::nonConstSingleton()->clearWDT();
			delay(500);
			setIsBLEConnected(true);
		    } else {
			PL("Disconnected!");
		        s_nextDisconnectTime = millis() + 5*1000;
			setIsBLEConnected(false);
		    }
		} else {
		    PL("Discovered disconnect; checking again in 5s");
		    setIsBLEConnected(false);
		    s_nextDisconnectTime = millis() + 5*1000;
		}
	    } else {
	        // see if reconnection has happened
	        if (ble.isConnected()) {
		    s_nextDisconnectTime = millis() + 70*1000;
		    PL("Reconnected!");
		    HivePlatform::nonConstSingleton()->clearWDT();
		    delay(500);
		    setIsBLEConnected(true);
		} else {
		    PL("Not connected; Scheduling a connection check in 5s");
		    s_nextDisconnectTime = millis() + 5*1000;
		}
	    }
	}
	s_mainState = LOOP;
    }
      break;
    }
    
}



void handleBLEInput(Adafruit_BluefruitLE_SPI &ble)
{
    if (s_rxLine == NULL) s_rxLine = new Str();

    TRACE("handleBLEInput; entry");
    ble.println("AT+BLEUARTRX");
    ble.readline();
    if (strcmp(ble.buffer, "OK") != 0) {
        TRACE("handleBLEInput; received something");
        s_rxLine->append(ble.buffer);

	// see if we have a full line
	const char *rx = s_rxLine->c_str();
	if (Parse::hasEOL(rx)) {
	    TRACE("handleBLEInput; have a line to process");
	    
            // A full line was collected -- figure out what it's for...
	    //D("Received: ");
	    //DL(s_rxLine->c_str());

	    while (Parse::hasEOL(rx) && (rx != NULL) && *rx) {
	        TRACE("handleBLEInput; processing line");
	        D("Considering: "); DL(rx);
		if (s_timestamp->isTimestampResponse(rx)) {
		    TRACE("handleBLEInput; is timestamp response");
		    rx = s_timestamp->processTimestampResponse(rx);
		    TRACE("handleBLEInput; done processing timestamp");
		} else {
		    TRACE("handleBLEInput; considering if response is for sensors or actuators");
		    int i = 0;
		    bool found = false;
		    while (!found && (s_sensors[i] != NULL)) {
		        if (s_sensors[i]->isMyResponse(rx)) {
			    //D("Sensor "); D(i); DL(" has claimed the response");
			    found = true;
			    rx = s_sensors[i]->processResponse(rx);
			}
			i++;
		    }
		    i = 0;
		    while (!found && (s_actuators[i] != NULL)) {
		        if (s_actuators[i]->isMyCommand(rx)) {
			    //D("Actuator "); D(i); DL(" has claimed the command");
			    found = true;
			    rx = s_actuators[i]->processCommand(rx);
			}
			i++;
		    }
		    
		    if (!found && (rx && *rx)) {
		        TRACE("handleBLEInput; determined line is not for sensors or actuators");
			
		        // assume it's randomly entered text -- report it, then try advancing to next line
			if (Parse::hasEOL(rx)) {
			    P("[Ignoring to EOL] "); PL(rx);
			    rx = Parse::consumeToEOL(rx);
			    if (*rx) 
			        P("[Continue with] "); PL(rx);
			} else {
			    P("[Unprocessed] "); PL(rx);
			}
		    } else {
		        TRACE("handleBLEInput; done handling sensor or actuator line");
		    }
		}
	    }

	    if ((rx == NULL) || (*rx == 0)) {
	        // reset s_rxLine
	        TRACE("handleBLEInput; reset s_rxLine");
		s_rxLine->clear();
	    } else {
	        TRACE("handleBLEInput; truncating s_rxLine to just the unprocessed part");
		Str holder(rx);
		*s_rxLine = holder;
		D("Left with a partial line: "); DL(s_rxLine->c_str());
	    }
	} else {
	    TRACE("handleBLEInput; received partial line");
	    D("Received partial line: "); DL(rx);
	}

	ble.waitForOK();
    }
}


/* STATIC */
bool setIsBLEConnected(bool v)
{
    bool r = s_connected;
    s_connected = v;
    digitalWrite(CONNECTED_LED, v ? HIGH : LOW);
    return r;
}


