#include <AudioActuator.h>

#include <Arduino.h>

//#define HEADLESS
//#define NDEBUG


#include <Trace.h>
#include <BleStream.h>
#include <sine.h>

#include "Adafruit_BluefruitLE_SPI.h"

#include <str.h>
#include <strutils.h>


/* STATIC */
const char *AudioActuator::sStateStrings[] = {"Idle", "RecordingRequested", "Recording", "RecordingDone"};

#define BUFSZ 128

AudioActuator::AudioActuator(const char *name, unsigned long now, BleStream *bleStream)
  : Actuator(name, now), mDuration(0), mStopTime(0), mBle(bleStream),
    mInputEnabled(true), 
    mState(Idle), mSine(new Sine(800, 20000, 368, 368+64))
{
    mBufs[0] = new unsigned short[BUFSZ];
    mBufs[1] = new unsigned short[BUFSZ];
    mCurrBuf = mBufs[0];
    mIndex = 0;
}


AudioActuator::~AudioActuator()
{
    delete mSine;
}


bool AudioActuator::isItTimeYet(unsigned long now) const
{
    TF("AudioActuator::isItTimeYet");
    
    switch (mState) {
    case Idle:
        return false;
    case RecordingRequested:
        TRACE("recording requested");
        return true;
    case Recording:
        return (now >= mStopTime);
    default: {
        TRACE("Unexpected state");
	TRACE(sStateStrings[mState]);
	return false;
    }
    }
}

void AudioActuator::scheduleNextAction(unsigned long now)
{
    TF("AudioActuator::scheduleNextAction");
    
    switch (mState) {
    case RecordingRequested:
        mStopTime = now + mDuration;
        TRACE("transitioning to Recording");
        mState = Recording;
	mSampleCnt = 0;
	break;
    case Recording:
        mState = RecordingDone;
	TRACE("transitioning to RecordingDone");
	break;
    case RecordingDone:
        mState = Idle;
        TRACE("transitioning to Idle");
	break;
    default: {
        TRACE("Unexpected state");
	TRACE(sStateStrings[mState]);
    }
    }
}


void AudioActuator::act(Adafruit_BluefruitLE_SPI &ble)
{
    TF("AudioActuator::act");
    
    switch (mState) {
    case ReadyToRecord:
        mInputEnabled = mBle->setEnableInput(false);
	streamStart(mBle->getBleImplementation(), mDuration);
	mState = Recording;
	break;
    case RecordingDone:
	streamStop(mBle->getBleImplementation());
        mBle->setEnableInput(mInputEnabled);
	mState = Idle;
	break;
    default: {
        TRACE("Unexpected state");
	TRACE(sStateStrings[mState]);
    }
    }
}


bool AudioActuator::isMyCommand(const Str &rsp) const
{
    TF("AudioActuator::isMyCommand");
    TRACE("entry");
    const char *token = "action|";
    const char *crsp = rsp.c_str();
    
    if (strncmp(crsp, token, strlen(token)) == 0) {
        crsp += strlen(token);
	if (strncmp(crsp, getName(), strlen(getName())) == 0) {
	    crsp += mName->len();
	    token = "|";
	    bool isMatch = (strncmp(crsp, token, strlen(token)) == 0);
	    TRACEDUMP(isMatch? "matches" : "mismatch");
	    return isMatch;
	}
    } else {
        TRACE("doesn't match");
    }
    
    return false;
}


void AudioActuator::processCommand(Str *msg)
{
    TF("AudioActuator::processCommand");
    
    switch (mState) {
    case Idle: {
        const char *token = "action|";
	const char *cmsgAtDuration = msg->c_str() + strlen(token) + strlen(getName()) + 1;
    
	int newDuration = atoi(cmsgAtDuration);

	*msg = cmsgAtDuration;
	StringUtils::consumeNumber(msg);
	StringUtils::consumeToEOL(msg);
    
	bool captureRequested = (newDuration > 0) && (newDuration < 60*1000);
	// must be positive #ms and < 60s
	
	if (captureRequested) {
	    TRACE("valid duration requested");
	    TRACE(newDuration);
	    mDuration = newDuration;
	    mState = RecordingRequested;
	} else {
	    TRACE("invalid duration requested");
	}
    }
      break;
    default: {
        TRACE("Unexpected state");
	TRACE(sStateStrings[mState]);
    }
    }
}

void AudioActuator::pulse(unsigned long now)
{
    // expecting to be called once every 50us

    if (mState == Recording) {
        //to see the pulse on the scope, enable "5" as an output and uncomment:
        const PinDescription &p = g_APinDescription[5];
	PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin);

	unsigned short s = mSine->sineSample(mSampleCnt++);
	mCurrBuf[mIndex++] = s;
	if (mIndex == BUFSZ) {
	  
	}
    }
}


void AudioActuator::streamStart(Adafruit_BluefruitLE_SPI *ble, int duration)
{
    TF("AudioActuator::streamStart");
    TRACE("informing the app");

    ble->print("AT+BLEUARTTX=");
    ble->print("cmd|stream|");
    ble->print(duration);
    ble->println("\\n");
    
    // check response status
    if ( ble->waitForOK() ) {
        TRACE("got ok after sending mic start/stop notification");
    } else {
        PL("didn't get ok after sending mic start/stop notification");
    }
}


void AudioActuator::streamStop(Adafruit_BluefruitLE_SPI *ble)
{
    TF("AudioActuator::streamStop");
    streamStart(ble, 0);
}


