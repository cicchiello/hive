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

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

//#define NDEBUG
#include <platformutils.h>
#include <cloudpipe.h>
#include <str.h>
#include <txqueue.h>
#include <freelist.h>
#include <SensorEntry.h>
#include <TimestampEntry.h>


#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)


#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif



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



// A small helper
void setup(void);
void loop(void);
bool pollChar(char buffer[]);
void error(const __FlashStringHelper*err) {
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
  while (!Serial);  // required for Flora & Micro
  delay(500);

  // setup WDT
  PlatformUtils::nonConstSingleton().initWDT(wdtEarlyWarningHandler);
  
  Serial.begin(115200);
  PL(F("Adafruit Bluefruit Command Mode Example"));
  PL(F("---------------------------------------"));

  /* Initialise the module */
  P(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
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
  ble.info();

  PL(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  PL(F("Then Enter characters to send to Bluefruit"));
  PL();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      PlatformUtils::nonConstSingleton().clearWDT();
      delay(500);
  }

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

#define INIT 0
#define CHECK_USERINPUT         (INIT+1)
#define LOOP                    CHECK_USERINPUT
#define CHECK_BLE_RX            (CHECK_USERINPUT+1)
#define REPORT_TEMP             (CHECK_BLE_RX+1)
#define QUEUE_TIMESTAMP_REQUEST (REPORT_TEMP+1)
#define ATTEMPT_SENSOR_POST     (QUEUE_TIMESTAMP_REQUEST+1)
#define ATTEMPT_TIMESTAMP_POST  (ATTEMPT_SENSOR_POST+1)
#define TEST_DISCONNECT         (ATTEMPT_TIMESTAMP_POST+1)


static int s_mainState = INIT;
static char s_userInput[BUFSIZE+1];

static bool s_haveTimestamp = false, s_requestedTimestamp = false;
static unsigned long s_timestampMark, s_secondsAtMark;
static Str s_rxLine;
static TxQueue<SensorEntry> *s_sensorTxQueue = NULL;
static FreeList<SensorEntry> *s_sensorEntryFreeList = NULL;
static TxQueue<TimestampEntry> *s_timestampTxQueue = NULL;
static FreeList<TimestampEntry> *s_timestampEntryFreeList = NULL;


void loop(void)
{
    unsigned long now = millis();
    switch (s_mainState) {
    case INIT: {
        D("BUFSIZE: ");
	DL(BUFSIZE);
	s_mainState = CHECK_USERINPUT;
	s_userInput[0] = 0;
	s_timestampEntryFreeList = new FreeList<TimestampEntry>();
	s_timestampTxQueue = new TxQueue<TimestampEntry>();
	s_sensorEntryFreeList = new FreeList<SensorEntry>();
	s_sensorTxQueue = new TxQueue<SensorEntry>();
	delay(500);
    }
      break;
      
    case CHECK_USERINPUT: {
        PlatformUtils::nonConstSingleton().clearWDT();
	
        // Check for user input
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

	// after one poll of the user's input, move on to the next state in the loop
	s_mainState = CHECK_BLE_RX;
    }
    break;
    
    case CHECK_BLE_RX: {
        // Check for incoming characters from Bluefruit
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
		
	        D("Received: ");
		DL(s_rxLine.c_str());
		
	        if (CloudPipe::singleton().isTimestampResponse(rx)) {
		    if (CloudPipe::singleton().processTimestampResponse(rx, &s_timestampMark)) {
		        s_haveTimestamp = true;
			s_secondsAtMark = (now+500)/1000;
			P("Have timestamp: "); PL(s_timestampMark);
			s_timestampTxQueue->receivedSuccessConfirmation(s_timestampEntryFreeList);
		    }
		} else if (CloudPipe::singleton().isSensorUploadResponse(rx)) {
		    if (CloudPipe::singleton().processSensorUploadResponse(rx)) {
		        s_sensorTxQueue->receivedSuccessConfirmation(s_sensorEntryFreeList);
		    } else {
		        s_sensorTxQueue->receivedFailureConfirmation();
		    }
		} else {
		    // assume it's randomly entered text
		    P(F("[Recv] ")); PL(rx);
		}
		
		// reset s_rxLine
		s_rxLine.clear();
	    }
	    
	    ble.waitForOK();
	}

	if (s_haveTimestamp)
	    s_mainState = REPORT_TEMP;
	else if (s_requestedTimestamp)
	    s_mainState = ATTEMPT_TIMESTAMP_POST;
	else
	    s_mainState = QUEUE_TIMESTAMP_REQUEST;
    }
    break;

    case QUEUE_TIMESTAMP_REQUEST: {
        if (now > 10000) {
	    DL("queueing a call for timestamp");

	    TimestampEntry *e = s_timestampEntryFreeList->pop();
	    if (e == 0) {
	        //DL("nothing popped off freelist; creating a new TimestampEntry");
	        e = new TimestampEntry();
	    } else {
	        //DL("using record from freelist");
	    }
		  
	    
	    s_timestampTxQueue->push(e);
	    s_requestedTimestamp = true;
	}
	s_mainState = LOOP;
    }
    break;
    
    case ATTEMPT_TIMESTAMP_POST: {
        //DL("Attempting timestamp post");
        s_timestampTxQueue->attemptPost(ble);
	s_mainState = LOOP;
    }
      break;
      
    case REPORT_TEMP: {
        // see if it's time to send a cpu temperature
        static unsigned long nextSampleTime = now + 20*1000;
	static int sendCnt = 0;

	if ( now >= nextSampleTime ) {
	    // crudely simulate a sensor by taking the BLE module's temperature
	    ble.println("AT+HWGETDIETEMP");
	    ble.readline();
	    Str temp(ble.buffer);
	    if (! ble.waitForOK() ) {
	        PL(F("Failed to send?"));
	    }
	    P("Measured: ");
	    PL(temp.c_str());
	    const char *sensorValueStr = temp.c_str();
		
	    nextSampleTime = now + 60*1000; // schedule the next one

	    unsigned long secondsSinceBoot = (now+500)/1000;
	    unsigned long secondsSinceMark = secondsSinceBoot - s_secondsAtMark;
	    unsigned long secondsSinceEpoch = secondsSinceMark + s_timestampMark;

	    char timestampStr[16];
	    sprintf(timestampStr, "%lu", secondsSinceEpoch);
	    
	    P("queueing an entry with timestamp=");
	    PL(secondsSinceEpoch);

	    SensorEntry *e = s_sensorEntryFreeList->pop();
	    if (e == 0) e = new SensorEntry("cputemp", sensorValueStr, timestampStr);
	    else e->set("cputemp", sensorValueStr, timestampStr);
	    
	    s_sensorTxQueue->push(e);
	}
	s_mainState = ATTEMPT_SENSOR_POST;
    }
      break;

    case ATTEMPT_SENSOR_POST: {
        //DL("Attempting sensor post");
        s_sensorTxQueue->attemptPost(ble);
	s_mainState = TEST_DISCONNECT;
    }
      break;
      
    case TEST_DISCONNECT: {
        static unsigned long nextDisconnectTime = 2*60*1000;
	static bool connected = true;
        if (now > nextDisconnectTime) {

	    if (connected) {
	        if (ble.isConnected()) {
		    PL("Disconnecting...");
		    ble.disconnect();
		    connected = false;
		
		    // see if reconnection has happened immediately
		    delay(500);
		    if (ble.isConnected()) {
		        nextDisconnectTime = millis() + 70*1000;
			PL("Reconnected!");
			delay(500);
			connected = true;
		    } else {
		        nextDisconnectTime = millis() + 5*1000;
		    }
		} else connected = false;
	    } else {
	        // see if reconnection has happened
	        if (ble.isConnected()) {
		    nextDisconnectTime = millis() + 70*1000;
		    PL("Reconnected!");
		    delay(500);
		    connected = true;
		} else {
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

