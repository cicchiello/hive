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
const char *AudioActuator::sStateStrings[] = {"Idle", "RecordingRequested", "InitRecording", "Recording", "RecordingDone"};

#define BUFSZ 128

AudioActuator::AudioActuator(const char *name, unsigned long now, BleStream *bleStream)
  : Actuator(name, now), mDuration(0), mStopTime(0), mBle(bleStream), mAcceptInput(false),
    mBleInputEnabled(true), mState(Idle), mSine(new Sine(800, 20000, 368, 368+64))
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

    AudioActuator *nonConstThis = (AudioActuator*) this;
    nonConstThis->mAcceptInput = true;
    
    switch (mState) {
    case Idle:
        return false;
    case RecordingRequested:
        TRACE("recording requested");
        return true;
    case Recording:
        return (now >= mStopTime);
    default: {
        TRACE2("Unexpected state: ", sStateStrings[mState]);
	return false;
    }
    }
}

void AudioActuator::scheduleNextAction(unsigned long now)
{
    TF("AudioActuator::scheduleNextAction");
    TRACE2("mState: ",sStateStrings[mState]);
    
    switch (mState) {
    case RecordingRequested:
        mStopTime = now + mDuration;
        TRACE("transitioning to InitRecording");
        mState = InitRecording;
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
        TRACE2("Unexpected state: ", sStateStrings[mState]);
    }
    }
}


void AudioActuator::act(Adafruit_BluefruitLE_SPI &ble)
{
    TF("AudioActuator::act");
    TRACE2("mState: ",sStateStrings[mState]);
    
    switch (mState) {
    case InitRecording: {
        mBleInputEnabled = mBle->setEnableInput(false);
	Str msg;
	prepareAppMsg(&msg, mDuration);
	postToApp(msg.c_str(), *mBle->getBleImplementation());
	mState = Recording;
        TRACE("transitioning to Recording");
    }
	break;
    case RecordingDone: {
	mState = Idle;
        Str msg;
	prepareAppMsg(&msg, 0);
	postToApp(msg.c_str(), *mBle->getBleImplementation());
	TRACE2("calling mBle->setEnableInput with: ",(mBleInputEnabled ? "true" : "false"));
        mBle->setEnableInput(mBleInputEnabled);
        TRACE("transitioning to Idle");
    }
	break;
    default: {
        TRACE2("Unexpected state: ",sStateStrings[mState]);
    }
    }
}


void AudioActuator::postToApp(const char *msg, Adafruit_BluefruitLE_SPI &ble)
{
    TF("AudioActuator::postToApp");
    TRACE("informing the app");
    ble.println(msg);

    // check response status
    if ( ble.waitForOK() ) {
        TRACE("got ok");
    } else {
        PL("didn't get ok");
    }
}


bool AudioActuator::isMyCommand(const Str &rsp) const
{
    TF("AudioActuator::isMyCommand");
    TRACE("entry");
    if (!mAcceptInput)
        return false;
    
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
	    mIndex = 0;
	}
    }
}


void AudioActuator::prepareAppMsg(Str *msg, int duration)
{
    TF("AudioActuator::streamStart");
    TRACE("preparing msg for the app");

    char buf[10];
    sprintf(buf, "%d", duration);
    
    *msg = "AT+BLEUARTTX=";
    msg->append("cmd|");
    msg->append(getName());
    msg->append("|STREAM|");
    msg->append(buf);
    msg->append("\\n");

    TRACE2("will send: ", msg->c_str());
}


