#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BluefruitLE_SPI.h"

#include "BluefruitConfig.h"

//#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>

#include <cloudpipe.h>
#include <Timestamp.h>
#include <BleStream.h>

#include <AudioActuator.h>

#include <Actuator.h>

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


#define INIT 0
#define CHECK_BLE_RX            (INIT+1)
#define LOOP                    CHECK_BLE_RX
#define QUEUE_TIMESTAMP_REQUEST (CHECK_BLE_RX+1)
#define VISIT_ACTUATOR          (QUEUE_TIMESTAMP_REQUEST+1)
#define ATTEMPT_TIMESTAMP_POST  (VISIT_ACTUATOR+1)
#define TEST_CONNECTION         (ATTEMPT_TIMESTAMP_POST+1)


static const char *ResetCause = "unknown";
static int s_mainState = INIT;

static int s_currActuator = -1;
static bool s_connected = false;
static unsigned long s_nextConnectCheckTime = 0;
static BleStream *s_bleStream = NULL;
static bool s_haveTimestamp = false;
static Timestamp *s_timestamp = NULL;
static class Sensor *s_unusedSensors[10];
static Actuator *s_actuators[10];




/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
    TF("::setup");
    
    // capture the reason for previous reset so I can inform the app
    ResetCause = HivePlatform::singleton()->getResetCause(); 
    
    delay(500);

    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
  
    pinMode(19, INPUT);               // used for preventing runnaway on reset
    while (digitalRead(19) == HIGH) {
        delay(500);
	PL("within the runnaway prevention loop");
    }
    PL("digitalRead(19) returned LOW");

    //pinMode(5, OUTPUT);         // for debugging: used to indicate ISR invocations for motor drivers
    
    delay(500);

    PL("audio.t debug console");
    PL("---------------------------------------");

    /* Initialise the module */
    P("Initialising the Bluefruit LE module: ");

    if ( !ble.begin(VERBOSE_MODE) )
    {
        ERR("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?");
    }

    delay(1500); // a little extra delay after the BLE reboot
    TRACE("ble initialized");
  
    PL("OK!");

    /* Perform a factory reset to make sure everything is in a known state */
    PL("Performing a factory reset: ");
    if ( ! ble.factoryReset() ){
        ERR("Couldn't factory reset");
    }
    delay(1500); // a little extra delay after the BLE reset

    /* Disable command echo from Bluefruit */
    ble.echo(false);

    PL("Requesting Bluefruit info:");
    /* Print Bluefruit information */
    ble.info();

    PL();
    PL("Awaiting connection");
    
    ble.verbose(false);  // debug info is a little annoying after this point!

    s_connected = false;
    
    HivePlatform::nonConstSingleton()->startWDT();
  
    /* Wait for connection */
    while (! ble.isConnected()) {
        HivePlatform::nonConstSingleton()->clearWDT();
	delay(500);
    }

    TRACE("Connected");
    PL("Connected");
    PL();
  
    s_connected = true;
    
    // Change Mode LED Activity
    ble.sendCommandCheckOK("AT+HWModeLED=MODE");
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
    case INIT: {
        TRACE("Initializing sensors and actuators");
	s_mainState = LOOP;
	CloudPipe::nonConstSingleton().initMacAddress(ble);
	s_timestamp = new Timestamp(ResetCause, "dev");

	s_actuators[0] = NULL;
	
	s_currActuator = 0;

	DL("Creating sensors and actuators");
	
	s_unusedSensors[0] = NULL;
	s_bleStream = new BleStream(&ble, s_timestamp, s_unusedSensors, s_actuators);
	
	AudioActuator *audio = new AudioActuator("mic", now, s_bleStream);
	s_actuators[0] = audio;
	s_actuators[1] = NULL;
	
	PL("Sensors and Actuators initialized;");
	TRACE("Sensors and Actuators initialized;");

	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_20K(audio->getPulseGenConsumer());
	HivePlatform::nonConstSingleton()->pulseGen_20K_init();
	PL("PulseGenerators initialized;");
	
	// wait a bit for comms to settle
	delay(500);
	TRACE("Initializations done;");
    }
      break;
      
    case CHECK_BLE_RX: {
        TRACE("checking for BLE rx");
	HivePlatform::nonConstSingleton()->clearWDT();
	
        // Check for incoming transmission from Bluefruit
	s_bleStream->processInput();

	if (s_timestamp->haveTimestamp()) {
	    if (!s_haveTimestamp) {
	        PL("Have timestamp");
		s_haveTimestamp = true;
	    }
	    s_mainState = VISIT_ACTUATOR;
	} else if (s_timestamp->haveRequestedTimestamp()) {
	    s_mainState = ATTEMPT_TIMESTAMP_POST;
	} else
	    s_mainState = QUEUE_TIMESTAMP_REQUEST;
    }
    break;

    case QUEUE_TIMESTAMP_REQUEST: {
        if (now > 10000) {
	    TRACE("QUEUE_TIMESTAMP_REQUEST");
	    PL("QUEUE_TIMESTAMP_REQUEST");
	    //DL("timestamp enqueueRequest");
	    s_timestamp->enqueueRequest();
	}
	s_mainState = LOOP;
    }
    break;
    
    case ATTEMPT_TIMESTAMP_POST: {
        PL("Attempting timestamp post");
        TRACE("Attempting timestamp post");
        s_timestamp->attemptPost(ble);
	s_mainState = LOOP;
    }
      break;

    case VISIT_ACTUATOR: {
        // see if it's time to sample
        TRACE("visiting actuator");
        if (s_actuators[s_currActuator] && s_actuators[s_currActuator]->isItTimeYet(now)) {
	    //PL("an actuator is ready");
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
	if (now > s_nextConnectCheckTime) {
	    TRACE("testing connection");
	    if (s_connected) {
	        if (!ble.isConnected()) {
		    PL("Discovered disconnect; checking again in 5s");
		    s_connected = false;
		}
	    } else {
	        // see if reconnection has happened
	        if (ble.isConnected()) {
		    PL("Reconnected!");
		    HivePlatform::nonConstSingleton()->clearWDT();
		    
		    ble.println("AT+BLEUARTTX=noop\\n");
		    if ( ble.waitForOK() ) {
		        TRACE("got ok");
		    } else {
		        PL("didn't get ok");
		    }
		    
		    delay(500);
		    s_connected = true;
		} else {
		    PL("Not connected; Scheduling a connection check in 5s");
		}
	    }
	    s_nextConnectCheckTime = millis() + 5*1000;
	}
	s_mainState = LOOP;
    }
      break;
      
    }
    
}

