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


#include <hive_platform.h>
#define TRACE(msg) HivePlatform::singleton()->trace(msg)
#define ERR(msg) HivePlatform::singleton()->error(msg)


#include <cloudpipe.h>
#include <BleStream.h>

#include <AudioActuator.h>


/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


#define INIT 0
#define CHECK_BLE_RX            (INIT+1)
#define VISIT_ACTUATOR          (CHECK_BLE_RX+1)
#define LOOP                    CHECK_BLE_RX


static const char *ResetCause = "unknown";
static int s_mainState = INIT;

#define MAX_ACTUATORS 20
//static int s_sensorSampleState = 0;
//static int s_currSensor = -1;
//static int s_currActuator = -1;
//static bool s_connected = false;
//static unsigned long s_nextConnectCheckTime = 0;
//static unsigned long s_nextDisconnectTime = 2*60*1000;
static BleStream *s_bleStream = NULL;
class Timestamp *s_unusedTimestamp = NULL;
static Actuator *s_actuators[MAX_ACTUATORS];


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

    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
  
    pinMode(19, INPUT);               // used for preventing runnaway on reset
    while (digitalRead(19) == HIGH) {
        delay(500);
	PL("within the runnaway prevention loop");
    }
    PL("digitalRead(19) returned LOW");
    P("RCAUSE: "); PL(ResetCause);

    pinMode(5, OUTPUT); // for debugging: used to indicate ISR invocations for pulse generator
    
    delay(500);

    PL("audio.t debug console");
    PL("-------------------------");


    /* Initialise the module */
    P("Initialising the Bluefruit LE module: ");

    if ( !ble.begin(true) )
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

    HivePlatform::nonConstSingleton()->startWDT();
  
    /* Wait for connection */
    while (! ble.isConnected()) {
        HivePlatform::nonConstSingleton()->clearWDT();
	delay(500);
    }

    TRACE("Connected");
    PL("Connected");
    PL();
  
    ble.sendCommandCheckOK("AT+HWModeLED=MODE" );
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
        TRACE("Initializing sensors and actuators");
	s_mainState = LOOP;
	CloudPipe::nonConstSingleton().initMacAddress(ble);
	
//	s_currSensor = 0;
//	s_currActuator = 0;

	DL("Creating sensors and actuators");

	Sensor **s_unusedSensors = NULL;
	s_bleStream = new BleStream(&ble, s_unusedTimestamp, s_unusedSensors, s_actuators);
	
//	AudioActuator *audio = new AudioActuator("mic", now, s_bleStream);
//	s_actuators[0] = audio;
	s_actuators[0] = NULL;
	s_actuators[1] = NULL;

	PL("Sensors initialized;");
	TRACE("Sensors initialized;");

//	HivePlatform::nonConstSingleton()->registerPulseGenConsumer_20K(audio->getPulseGenConsumer());
//	HivePlatform::nonConstSingleton()->pulseGen_20K_init();
	PL("PulseGenerators initialized;");
	
	// wait a bit for comms to settle
	delay(500);
	
	DL("INIT state done");
    }
      break;
      
    case CHECK_BLE_RX: {
        TRACE("checking for BLE rx");
	HivePlatform::nonConstSingleton()->clearWDT();
	
        // Check for incoming transmission from Bluefruit
	s_bleStream->processInput();

	s_mainState = VISIT_ACTUATOR;
    }
    break;

    case VISIT_ACTUATOR: {
        HivePlatform::singleton()->trace("visiting actuator");
	HivePlatform::nonConstSingleton()->clearWDT();
	
        // see if it's time to sample
        TRACE("visiting actuator");
        if (s_actuators[0] && s_actuators[0]->isItTimeYet(now)) {
	    s_actuators[0]->scheduleNextAction(now);

	    s_actuators[0]->act(ble);
	    
	    TRACE("done visiting actuator");
	}

	s_mainState = LOOP;
    }
      break;

    default:
      HivePlatform::singleton()->error("Unknown s_mainState in loop; quitting");
    };
}
