/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BluefruitLE_SPI.h"

#include "BluefruitConfig.h"


#define HEADLESS
#define NDEBUG

#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#define assert(c,msg) if (!(c)) {PL("ASSERT"); WDT_TRACE(msg); while(1);}


#include <platformutils.h>
#include <cloudpipe.h>
#include <str.h>
#include <txqueue.h>
#include <Timestamp.h>
//#include <RxBLE.h>

#include <CpuTempSensor.h>
#include <TempSensor.h>
#include <HumidSensor.h>
#include <StepperMonitor.h>
#include <StepperActuator.h>



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


static int s_mainState = INIT;

#define BLE_INPUT_POLL_RATE 1000
#define MAX_SENSORS 20
#define MAX_ACTUATORS 20
static Timestamp *s_timestamp = 0;
static int s_sensorSampleState = 0;
static Sensor *s_sensors[MAX_SENSORS];
static Actuator *s_actuators[MAX_ACTUATORS];
static int s_currSensor = -1;
static int s_currActuator = -1;
static bool s_connected = false;



static bool setIsBLEConnected(bool v);
static void handleBLEInput(Adafruit_BluefruitLE_SPI &ble);
  
static bool pollChar(char buffer[]);
static void error(const __FlashStringHelper*err) {
  PL(err);
  while (1);
}

static void wdtEarlyWarningHandler()
{
    // first, prevent the WDT from doing a full system reset by resetting the timer
    PlatformUtils::nonConstSingleton().clearWDT();

    PL("wdtEarlyWarningHandler; BARK!");
    PL("");

    if (PlatformUtils::s_traceStr != NULL) {
        P("WDT Trace message: ");
	PL(PlatformUtils::s_traceStr);
    } else {
        PL("No WDT trace message registered");
    }
    
    // Next, do a more useful system reset
    PlatformUtils::nonConstSingleton().resetToBootloader();
}



/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
#ifndef HEADLESS
    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
#endif
  
    pinMode(13, OUTPUT);

    delay(500);

    // setup WDT
    PlatformUtils::nonConstSingleton().initWDT(wdtEarlyWarningHandler);
  
    PL(F("Hive Controller debug console"));
    PL(F("---------------------------------------"));

    /* Initialise the module */
    P(F("Initialising the Bluefruit LE module: "));

#ifndef HEADLESS
    if ( !ble.begin(VERBOSE_MODE) )
    {
        error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
    }
#else
    if ( !ble.begin(false) )
    {
        error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
    }
#endif
  
    PL( F("OK!") );

    if ( FACTORYRESET_ENABLE )
    {
        /* Perform a factory reset to make sure everything is in a known state */
        PL(F("Performing a factory reset: "));
	if ( ! ble.factoryReset() ){
	    error(F("Couldn't factory reset"));
	}
    }

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
  
    /* Wait for connection */
    while (! ble.isConnected()) {
        PlatformUtils::nonConstSingleton().clearWDT();
	delay(500);
    }

    setIsBLEConnected(true);
    PL("Connected");
    PL();
  
    // LED Activity command is only supported from 0.6.6
    if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
    {
        // Change Mode LED Activity
        PL(F("******************************"));
	PL(F("Change LED activity to " MODE_LED_BEHAVIOUR));
	ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
	PL(F("******************************"));
    }
}


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
        D("BUFSIZE: ");
	DL(BUFSIZE);
	for (int i = 0; i < MAX_SENSORS; i++) {
	    s_sensors[i] = NULL;
	}
	for (int i = 0; i < MAX_ACTUATORS; i++) {
	    s_actuators[i] = NULL;
	}
	s_mainState = LOOP;
	CloudPipe::nonConstSingleton().initMacAddress(ble);
	s_timestamp = new Timestamp();

	// register sensors
	s_currSensor = 0;
	s_currActuator = 0;
	s_sensors[0] = new CpuTempSensor(now, ble);
	s_sensors[1] = new TempSensor(now);
	s_sensors[2] = new HumidSensor(now);
	StepperActuator *motor0 = new StepperActuator("motor0", 1, now, 40);
	s_sensors[3] = new StepperMonitor(*motor0, now);
	s_actuators[0] = motor0;
//	s_sensors[4] = new StepperMonitor("motor1", now, -20);
//	s_sensors[5] = new StepperMonitor("motor2", now, 30);

	PL("Sensors initialized;");
  
	// wait a bit for comms to settle
	delay(500);
    }
      break;
      
    case CHECK_BLE_RX: {
        WDT_TRACE("checking for BLE rx");
        PlatformUtils::nonConstSingleton().clearWDT();
	
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
        WDT_TRACE("Attempting timestamp post");
        s_timestamp->attemptPost(ble);
	s_mainState = LOOP;
    }
      break;
      
    case SENSOR_SAMPLE: {
        WDT_TRACE("Sensor sample state");
        switch (s_sensorSampleState) {
	case 0: {
	    // see if it's time to sample
	    if (s_sensors[s_currSensor]->isItTimeYet(now)) {
	        WDT_TRACE("starting sampling sensor");
		D("starting sampling sensor ");
		D(s_currSensor);
		D(" (now == ");
		D(now);
		DL(")");

		s_sensors[s_currSensor]->scheduleNextSample(now);

		Str sensorValueStr;
		if (s_sensors[s_currSensor]->sensorSample(&sensorValueStr)) {
		    Str timestampStr;
		    s_timestamp->toString(now, &timestampStr);
	    
		    //D("queueing an entry with timestamp=");
		    //DL(timestampStr.c_str());

		    s_sensors[s_currSensor]->enqueueRequest(sensorValueStr.c_str(),
							    timestampStr.c_str());
		    D("done sampling sensor (now == ");
		    D(millis());
		    DL(")");
		    WDT_TRACE("done sampling sensor");
		
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

	case 1: {
	    D("continuing sampling sensor ");
	    D(s_currSensor);
	    D(" (now == ");
	    D(now);
	    DL(")");
	    Str sensorValueStr;
	    if (s_sensors[s_currSensor]->sensorSample(&sensorValueStr)) {
	        DL("have value");
	        Str timestampStr;
		s_timestamp->toString(now, &timestampStr);
	    
		//D("queueing an entry with timestamp=");
		//DL(timestampStr.c_str());

		s_sensors[s_currSensor]->enqueueRequest(sensorValueStr.c_str(),
							timestampStr.c_str());
		D("done sampling sensor (now==");
		D(millis());
		DL(")");
		WDT_TRACE("done sampling sensor");
		
		s_mainState = ATTEMPT_SENSOR_POST;
		s_sensorSampleState = 0;
	    } else {
	        DL("still don't have value");
	        s_mainState = VISIT_ACTUATOR;
	    }
	}
	  break;
	}
	
    }
      break;

    case ATTEMPT_SENSOR_POST: {
        WDT_TRACE("Attempting sensor post");
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
        WDT_TRACE("visiting actuator");
        if (s_actuators[s_currActuator]->isItTimeYet(now)) {
	    s_actuators[s_currActuator]->scheduleNextAction(now);

	    s_actuators[s_currActuator]->act();
	    
	    s_currActuator++;
	    if (s_actuators[s_currActuator] == NULL) {
	        s_currActuator = 0;
	    }
	
	    WDT_TRACE("done visiting actuator");
	}

	s_mainState = TEST_CONNECTION;
	
    }
      break;

    case TEST_CONNECTION: {
        static unsigned long nextConnectCheckTime = 0;
	WDT_TRACE("testing connection");
	if (now > nextConnectCheckTime) {
	    if (s_connected) {
	        if (!ble.isConnected()) {
		    PL("Discovered disconnect; checking again in 5s");
		    setIsBLEConnected(false);
		}
	    } else {
	        // see if reconnection has happened
	        if (ble.isConnected()) {
		    PL("Reconnected!");
		    PlatformUtils::nonConstSingleton().clearWDT();
		    delay(500);
		    setIsBLEConnected(true);
		} else {
		    PL("Not connected; Scheduling a connection check in 5s");
		}
	    }
	    nextConnectCheckTime = millis() + 5*1000;
	}
	s_mainState = LOOP;
    }
      break;
      
    case TEST_DISCONNECT: {
        static unsigned long nextDisconnectTime = 2*60*1000;
        if (now > nextDisconnectTime) {

	    if (s_connected) {
	        if (ble.isConnected()) {
		    PL("Disconnecting...");
		    ble.disconnect();
		    setIsBLEConnected(false);
		
		    // see if reconnection has happened immediately
		    PlatformUtils::nonConstSingleton().clearWDT();
		    delay(500);
		    if (ble.isConnected()) {
		        nextDisconnectTime = millis() + 70*1000;
			PL("Reconnected!");
			PlatformUtils::nonConstSingleton().clearWDT();
			delay(500);
			setIsBLEConnected(true);
		    } else {
			PL("Disconnected!");
		        nextDisconnectTime = millis() + 5*1000;
			setIsBLEConnected(false);
		    }
		} else {
		    PL("Discovered disconnect; checking again in 5s");
		    setIsBLEConnected(false);
		    nextDisconnectTime = millis() + 5*1000;
		}
	    } else {
	        // see if reconnection has happened
	        if (ble.isConnected()) {
		    nextDisconnectTime = millis() + 70*1000;
		    PL("Reconnected!");
		    PlatformUtils::nonConstSingleton().clearWDT();
		    delay(500);
		    setIsBLEConnected(true);
		} else {
		    PL("Not connected; Scheduling a connection check in 5s");
		    nextDisconnectTime = millis() + 5*1000;
		}
	    }
	}
	s_mainState = LOOP;
    }
      break;
    }
    
}


/**************************************************************************/
/*!
    @brief  Checks for user input (via the Serial Monitor)
*/
/**************************************************************************/
bool pollChar(char buffer[])
{
    if (Serial.available()) {
        PL("Poll detected a character");
        int len = strlen(buffer);
        Serial.readBytes(buffer + len, 1);
	buffer[len+1] = 0;
	return true;
    } else {
        return false;
    }
}


void handleBLEInput(Adafruit_BluefruitLE_SPI &ble)
{
    static Str s_rxLine;
    
    ble.println("AT+BLEUARTRX");
    ble.readline();
    if (strcmp(ble.buffer, "OK") != 0) {
        s_rxLine.append(ble.buffer);
	int len = s_rxLine.len();

	// see if we have a full line
	char *rx = (char*) s_rxLine.c_str();
	if ((rx[len-2] == '\\') && (rx[len-1] == 'n')) {
	      
	    // A full line was collected -- figure out what it's for...
	    rx[len-2] = 0;
		
	    //D("Received: ");
	    //DL(s_rxLine.c_str());

	    if (s_timestamp->isTimestampResponse(rx)) {
	        s_timestamp->processTimestampResponse(rx);
	    } else {
	        bool found = false;
		int i = 0;
		while (!found && (s_sensors[i] != NULL)) {
		    if (s_sensors[i]->isMyResponse(rx)) {
		        found = true;
			s_sensors[i]->processResponse(rx);
		    }
		}
		if (!found) {
		    // assume it's randomly entered text -- report it
		    P(F("[Recv] ")); PL(rx);
		}
	    }
	    
	    // reset s_rxLine
	    s_rxLine.clear();
	}
	    
	ble.waitForOK();
    }
}


/* STATIC */
bool setIsBLEConnected(bool v)
{
    bool r = s_connected;
    s_connected = v;
    digitalWrite(13, v ? HIGH : LOW);
    return r;
}
