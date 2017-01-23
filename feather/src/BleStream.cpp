#include <BleStream.h>

#include <Timestamp.h>
#include <Sensor.h>
#include <Actuator.h>

#include "Adafruit_BluefruitLE_SPI.h"


#define HEADLESS
#define NDEBUG

#include <Trace.h>
#include <hive_platform.h>


#include <str.h>
#include <strutils.h>


static Str *s_rxLine = NULL;

static BleStream *s_singleton = NULL;


BleStream::BleStream(Adafruit_BluefruitLE_SPI *bleStream,
		     Timestamp *th, Sensor **sensors, Actuator **actuators)
  : mBle(bleStream), mTimestamp(th),
    mSensors(sensors), mActuators(actuators), mInputEnabled(true)
{
    TF("BleStream::BleStream");
    if (s_singleton != NULL) {
        ERR(HivePlatform::singleton()->error("BleStream::BleStream; there can be only one!"));
    }

    s_singleton = this;
}


#define MY_BLE_BUFSIZE 512
void BleStream::processInput()
{
    static char buf[MY_BLE_BUFSIZE];
    TF("BleStream::processInput");
    
    if (s_rxLine == NULL)
        s_rxLine = new Str();

    if (!mInputEnabled) {
        TRACE("input is disabled");
        return;
    }
    
    mBle->println("AT+BLEUARTRX");
    int len = mBle->readline(buf, MY_BLE_BUFSIZE);
    if (len == MY_BLE_BUFSIZE) {
        // OOOOHHHH NOOOOO!  probable overflow
        P("handleBLEInput; just read ");
	P(len);
	PL(" chars into buffer");
    }
//    TRACE("checking input");
    
    if (strcmp(buf, "OK") != 0) {
        PL("got something...");
        TRACE("received something");
        s_rxLine->append(buf);

	// see if we have a full line
	if (StringUtils::hasEOL(*s_rxLine)) {
	    Str lineCache(*s_rxLine);
	    TRACE("handleBLEInput; have a line to process");
	    
            // A full line was collected -- figure out what it's for...
	    D("Received: ");
	    DL(s_rxLine->c_str());

	    while (s_rxLine->len() && StringUtils::hasEOL(*s_rxLine)) {
	        TRACE("handleBLEInput; processing line");
	        //D("Considering: "); DL(s_rxLine->c_str());
		if (mTimestamp && mTimestamp->isTimestampResponse(*s_rxLine)) {
		    TRACE("handleBLEInput; is timestamp response");
		    mTimestamp->processTimestampResponse(s_rxLine);
		    TRACE("handleBLEInput; done processing timestamp");
		} else {
		    TRACE("handleBLEInput; considering if response is for sensors or actuators");
		    int i = 0;
		    bool found = false;
		    while (!found && (mSensors[i] != NULL)) {
		        if (mSensors[i]->isMyResponse(*s_rxLine)) {
			    D("Sensor "); D(i); DL(" has claimed the response");
			    found = true;
			    mSensors[i]->processResponse(s_rxLine);
			}
			i++;
		    }
		    i = 0;
		    while (!found && (mActuators[i] != NULL)) {
		        if (mActuators[i]->isMyCommand(*s_rxLine)) {
			    D("Actuator "); D(i); DL(" has claimed the command");
			    found = true;
			    mActuators[i]->processCommand(s_rxLine);
			}
			i++;
		    }
		    
		    if (!found && s_rxLine->len()) {
		        TRACE("handleBLEInput; determined line is not for sensors or actuators");
			
		        // assume it's randomly entered text -- report it, then try advancing to next line
			if (StringUtils::hasEOL(*s_rxLine)) {
			    if (!StringUtils::isAtEOL(*s_rxLine)) {
			        P("[Ignoring to EOL; here's original line] "); PL(lineCache.c_str());
				P("[Ignoring to EOL; and here's what's left] "); PL(s_rxLine->c_str());
			    }
			    
			    StringUtils::consumeToEOL(s_rxLine);
			    if (s_rxLine->len())
			        P("[Continue with] "); PL(s_rxLine->c_str());
			} else {
			    P("[Unprocessed] "); PL(s_rxLine->c_str());
			}
		    } else {
		        TRACE("handleBLEInput; done handling sensor or actuator line");
		    }
		}
	    }

	    if (s_rxLine->len()) {
	        TRACE("handleBLEInput; left with a partial line");
		//D("Left with a partial line: "); DL(s_rxLine->c_str());
	    } else {
	        TRACE("handleBLEInput; entire line consumed");
	    }
	} else {
	    TRACE("handleBLEInput; received partial line");
	    //D("Received partial line: "); DL(s_rxLine->c_str());
	}

	mBle->waitForOK();
    }
}


bool BleStream::setEnableInput(bool v)
{
    bool r = mInputEnabled;
    mInputEnabled = v;
    return r;
}

