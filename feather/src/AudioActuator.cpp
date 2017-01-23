#include <AudioActuator.h>

#include <Arduino.h>

//#define HEADLESS
//#define NDEBUG


#include <Trace.h>
#include <BleStream.h>
#include <sine.h>

#include "Adafruit_BluefruitLE_SPI.h"

#include <hive_platform.h>
#include <platformutils.h>

#include <str.h>
#include <strutils.h>


/* STATIC */
const char *AudioActuator::sStateStrings[] = {"Idle", "RecordingRequested", "InitRecording", "Recording", "RecordingDone"};

#define BUFSZ 128

AudioActuator::AudioActuator(const char *name, unsigned long now, BleStream *bleStream)
  : Actuator(name, now), mDuration(0), mStopTime(0), mNextVisitTime(0),
    mBle(bleStream), mAcceptInput(false),
    mBleInputEnabled(true), mState(Idle), mSine(new Sine(800, 20000, 368, 368+64))
{
    mBufs[0] = new unsigned char[BUFSZ];
    mBufs[1] = new unsigned char[BUFSZ];
    mCurrBuf = mBufs[0];
    mBufToStream = NULL;
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
        return (now >= mNextVisitTime);
    default: {
        TRACE2("Unexpected state: ", sStateStrings[mState]);
	return false;
    }
    }
}


static int sPackets = 0;

void AudioActuator::scheduleNextAction(unsigned long now)
{
    TF("AudioActuator::scheduleNextAction");
    //TRACE2("mState: ",sStateStrings[mState]);
    
    switch (mState) {
    case RecordingRequested:
        mStopTime = now + mDuration;
	mNextVisitTime = now + 1;
        TRACE("transitioning to InitRecording");
        mState = InitRecording;
	mSampleCnt = 0;
	break;
    case Recording:
        if (now > mStopTime) {
	    mState = RecordingDone;
	    TRACE("transitioning to RecordingDone");
	} else {
	    mNextVisitTime = now+1;
	}
	break;
    case RecordingDone:
        mState = Idle;
        TRACE2("transitioning to Idle; packets so far: ", sPackets);
	break;
    default: {
        TRACE2("Unexpected state: ", sStateStrings[mState]);
    }
    }
}


void AudioActuator::act(Adafruit_BluefruitLE_SPI &ble)
{
    TF("AudioActuator::act");
    //TRACE2("mState: ",sStateStrings[mState]);
    
    switch (mState) {
    case InitRecording: {
        mBleInputEnabled = mBle->setEnableInput(false);
	Str msg;
	prepareAppMsg(&msg, mDuration);
	postToApp(msg.c_str(), *mBle->getBleImplementation());
	mState = Recording;
	Adafruit_BluefruitLE_SPI *ble = mBle->getBleImplementation();
	ble->setMode(BLUEFRUIT_MODE_DATA);
        TRACE("transitioning to Recording");
    }
	break;
	
    case Recording: {
        while (millis() < mStopTime) {
	    if (mBufToStream != NULL) {
	        unsigned char *buf = mBufToStream;
		mBufToStream = NULL;
		sPackets++;
		Adafruit_BluefruitLE_SPI *ble = mBle->getBleImplementation();
		ble->write(buf, 16);
//		ble->write(buf, BUFSZ*sizeof(unsigned char));
		PlatformUtils::nonConstSingleton().clearWDT();
	    }
	}
	mState = RecordingDone;
	TRACE("transitioning to RecordingDone");
	
	TRACE("transitioning to Idle");
	mState = Idle;
	Adafruit_BluefruitLE_SPI *ble = mBle->getBleImplementation();
	ble->setMode(BLUEFRUIT_MODE_COMMAND);
        Str msg;
	prepareAppMsg(&msg, 0);
	postToApp(msg.c_str(), *mBle->getBleImplementation());
	TRACE2("calling mBle->setEnableInput with: ",(mBleInputEnabled ? "true" : "false"));
        mBle->setEnableInput(mBleInputEnabled);
        TRACE2("transitioning to Idle; packets so far: ", sPackets);
    }
        break;
	
    case RecordingDone: {
	mState = Idle;
	Adafruit_BluefruitLE_SPI *ble = mBle->getBleImplementation();
	ble->setMode(BLUEFRUIT_MODE_COMMAND);
        Str msg;
	prepareAppMsg(&msg, 0);
	postToApp(msg.c_str(), *mBle->getBleImplementation());
	TRACE2("calling mBle->setEnableInput with: ",(mBleInputEnabled ? "true" : "false"));
        mBle->setEnableInput(mBleInputEnabled);
        TRACE2("transitioning to Idle; packets so far: ", sPackets);
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
	    TRACE(isMatch? "matches" : "mismatch");
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
    TF("AudioActuator::pulse");
    
    // expecting to be called once every 50us
    
    if (mState == Recording) {
        //to see the pulse on the scope, enable "5" as an output and uncomment:
        const PinDescription &p = g_APinDescription[5];
	PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin);

	unsigned short s = mSine->sineSample(mSampleCnt++);
	unsigned char bl = s&0xff;
	unsigned char bh = (s>>8)&0xff;
	mCurrBuf[mIndex++] = bl;
	mCurrBuf[mIndex++] = bh;
	if (mIndex >= BUFSZ) {
	    if (mBufToStream != NULL) {
	        TRACE2("buffer overflow; packets so far: ", sPackets);
	        ERR(HivePlatform::error("buffer overflow"));
	    }
	    
	    mBufToStream = mCurrBuf;
	    mCurrBuf = (mCurrBuf == mBufs[0] ? mBufs[1] : mBufs[0]);
	    
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


