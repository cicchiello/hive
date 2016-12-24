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


//#define HEADLESS
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


#include <platformutils.h>
#include <cloudpipe.h>
#include <str.h>
#include <txqueue.h>
#include <Timestamp.h>

#include <CpuTempSensor.h>
#include <TempSensor.h>
#include <HumidSensor.h>




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
#define CHECK_USERINPUT         (INIT+1)
#define LOOP                    CHECK_USERINPUT
#define CHECK_BLE_RX            (CHECK_USERINPUT+1)
#define SAMPLE_SENSOR           (CHECK_BLE_RX+1)
#define QUEUE_TIMESTAMP_REQUEST (SAMPLE_SENSOR+1)
#define ATTEMPT_SENSOR_POST     (QUEUE_TIMESTAMP_REQUEST+1)
#define ATTEMPT_TIMESTAMP_POST  (ATTEMPT_SENSOR_POST+1)
#define TEST_CONNECTION         (ATTEMPT_TIMESTAMP_POST+1)
#define TEST_DISCONNECT         (TEST_CONNECTION+1)


static int s_mainState = INIT;

#define MAX_SENSORS 20
static Timestamp *s_timestamp = 0;
static Sensor *s_sensors[MAX_SENSORS];
static int s_currSensor = -1;
static bool s_connected = false;



static bool setIsBLEConnected(bool v);
static void handleUserInput(Adafruit_BluefruitLE_SPI &ble);
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
	s_mainState = CHECK_USERINPUT;
	s_timestamp = new Timestamp();

	// register sensors
	s_currSensor = 0;
	s_sensors[0] = new CpuTempSensor(now, ble);
	s_sensors[1] = new TempSensor(now);
	s_sensors[2] = new HumidSensor(now);

	PL("Sensors initialized;");
  
	// wait a bit for comms to settle
	delay(500);
    }
      break;
      
    case CHECK_USERINPUT: {
        PlatformUtils::nonConstSingleton().clearWDT();
	
        // Check for user input
	handleUserInput(ble);

	// after a poll of the user's input, move on to the next state in the loop
	s_mainState = CHECK_BLE_RX;
    }
    break;
    
    case CHECK_BLE_RX: {
        // Check for incoming transmission from Bluefruit
        handleBLEInput(ble);

	if (s_timestamp->haveTimestamp())
	    s_mainState = SAMPLE_SENSOR;
	else if (s_timestamp->haveRequestedTimestamp())
	    s_mainState = ATTEMPT_TIMESTAMP_POST;
	else
	    s_mainState = QUEUE_TIMESTAMP_REQUEST;
    }
    break;

    case QUEUE_TIMESTAMP_REQUEST: {
        if (now > 10000) {
	    s_timestamp->enqueueRequest();
	}
	s_mainState = LOOP;
    }
    break;
    
    case ATTEMPT_TIMESTAMP_POST: {
        //DL("Attempting timestamp post");
        s_timestamp->attemptPost(ble);
	s_mainState = LOOP;
    }
      break;
      
    case SAMPLE_SENSOR: {
        // see if it's time to sample
        if (s_sensors[s_currSensor]->isItTimeYet(now)) {
	    s_sensors[s_currSensor]->scheduleNextSample(now);

	    Str timestampStr;
	    s_timestamp->toString(now, &timestampStr);
	    
	    Str sensorValueStr;
	    s_sensors[s_currSensor]->sensorSample(&sensorValueStr);
	    
	    D("queueing an entry with timestamp=");
	    DL(timestampStr.c_str());

	    s_sensors[s_currSensor]->enqueueRequest(sensorValueStr.c_str(), timestampStr.c_str());
	}

	s_mainState = ATTEMPT_SENSOR_POST;
	
    }
      break;

    case ATTEMPT_SENSOR_POST: {
        //DL("Attempting sensor post");
        s_sensors[s_currSensor]->attemptPost(ble);

	s_currSensor++;
	if (s_sensors[s_currSensor] == NULL) {
	    s_currSensor = 0;
	}
	
	s_mainState = TEST_CONNECTION;
    }
      break;
      
    case TEST_CONNECTION: {
        static unsigned long nextConnectCheckTime = 0;
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
		    delay(500);
		    if (ble.isConnected()) {
		        nextDisconnectTime = millis() + 70*1000;
			PL("Reconnected!");
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



void handleUserInput(Adafruit_BluefruitLE_SPI &ble)
{
    static char s_userInput[BUFSIZE+1];
    static bool s_userInputInitialized = false;
    if (!s_userInputInitialized) {
        s_userInput[0] = 0;
	s_userInputInitialized = true;
    }
    
    if (pollChar(s_userInput)) {
        // see if the character is the line terminator
        int len = strlen(s_userInput);
	if ((s_userInput[len-1] == '\n') || (s_userInput[len-1] == '\r')) {
	    // remove the trailing \n 'cause it confuses the AT command set of the BLE
	    s_userInput[len-1] = 0;
	      
	    // send string to Bluefruit
	    P("[Send] ");
	    PL(s_userInput);

	    ble.print("AT+BLEUARTTX=");
	    ble.print(s_userInput);
	    ble.println("\\n");

	    // check response stastus
	    if (! ble.waitForOK() ) {
	        PL(F("Failed to send?"));
	    }
		
	    // set the buffer back to empty
	    s_userInput[0] = 0;
	} else {
	    if (strlen(s_userInput) == BUFSIZE) {
	        // at buffer limit
	        P("[Send substring] ");
		PL(s_userInput);

		ble.print("AT+BLEUARTTX=");
		ble.print(s_userInput);
		ble.println("\\n");
	    }
	}
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
