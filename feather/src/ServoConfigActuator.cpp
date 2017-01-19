#include <ServoConfigActuator.h>

#include <Arduino.h>


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

#include <hive_platform.h>

#include <str.h>
#include <strutils.h>



#include <txqueue.h>
#include <freelist.h>

#define ERROR(msg)     HivePlatform::singleton()->error(msg)
#define TRACE(msg)     HivePlatform::singleton()->trace(msg)


static FreeList<ServoConfigActuator::QueueEntry> *s_servoConfigEntryFreeList = NULL;
static TxQueue<ServoConfigActuator::QueueEntry> *s_servoConfigTxQueue = NULL;


static FreeList<ServoConfigActuator::QueueEntry> *getFreelist()
{
    if (s_servoConfigEntryFreeList == NULL) {
        s_servoConfigEntryFreeList = new FreeList<ServoConfigActuator::QueueEntry>();
    }
    return s_servoConfigEntryFreeList;
}

static TxQueue<ServoConfigActuator::QueueEntry> *getQueue()
{
    if (s_servoConfigTxQueue == NULL) {
        s_servoConfigTxQueue = new TxQueue<ServoConfigActuator::QueueEntry>();
    }
    return s_servoConfigTxQueue;
}




ServoConfigActuator::ServoConfigActuator(const char *name, unsigned long now)
  : Actuator(name, now+10*1000), mTripTempC(37.7), mLowerLimitTicks(40), mUpperLimitTicks(43)
{
}

ServoConfigActuator::~ServoConfigActuator()
{
}

void ServoConfigActuator::act(class Adafruit_BluefruitLE_SPI &ble)
{
    DL("ServoConfigActuator::act called");
    if (getQueue()->getLen() == 0) {
        enqueueRequest();
    }
    attemptPost(ble);
}


bool ServoConfigActuator::isItTimeYet(unsigned long now)
{
    return now >= mNextActionTime;
}


void ServoConfigActuator::scheduleNextAction(unsigned long now)
{
    unsigned long deltaSeconds = 60;
    mNextActionTime = now + deltaSeconds*1000;
}


bool ServoConfigActuator::isMyCommand(const Str &msg) const
{
    DL("ServoConfigActuator::isMyResponse");
    const char *token = "action|";
    const char *cmsg = msg.c_str();
    
    if (strncmp(cmsg, token, strlen(token)) == 0) {
        cmsg += strlen(token);
	const char *name = getName();
	if (strncmp(cmsg, name, strlen(name)) == 0) {
	    cmsg += strlen(name);
	    token = "|temp|dir|minTicks|maxTicks|";
	    return (strncmp(cmsg, token, strlen(token)) == 0);
	}
    } 
    return false;
}


void ServoConfigActuator::processCommand(Str *msg)
{
    Str msgAtDir;

    // using otherwise-unnecessary scoping to make sure variables aren't unexpected used later
    {
        const char *token0 = "action|";
	const char *token1 = getName();
	const char *token2 = "|temp|dir|minTicks|maxTicks|";
	const char *cmsgAtTemp = msg->c_str() + strlen(token0) + strlen(token1) + strlen(token2);

	D(TAG("processCommand", "parsing: ").c_str()); DL(cmsgAtTemp);
	double newTripTempC = atoi(cmsgAtTemp);
	if (newTripTempC != mTripTempC) {
	    D(TAG("processCommand", "mTripTempC was ").c_str()); DL(mTripTempC);
	    D(TAG("processCommand", "new temp is ").c_str()); DL(newTripTempC);
	    mTripTempC = newTripTempC;
	}
	msgAtDir = cmsgAtTemp;
	StringUtils::consumeNumber(&msgAtDir);
    }

    Str msgAtMinTicks;
    bool haveDir = false;
    {
        const char *clockwiseToken = "|CW|";
	const char *counterclockwiseToken = "|CCW|";
	const char *cmsgAtDir = msgAtDir.c_str();
	
	if (strncmp(cmsgAtDir, clockwiseToken, strlen(clockwiseToken)) == 0) {
	    D(TAG("processCommand", "mIsClockwise was ").c_str()); DL(mIsClockwise ? "CW" : "CCW");
	    DL(TAG("processCommand", "new isClockwise is CW").c_str());
	    mIsClockwise = true;
	    msgAtMinTicks = cmsgAtDir + strlen(clockwiseToken);
	    haveDir = true;
	} else if (strncmp(cmsgAtDir, counterclockwiseToken, strlen(counterclockwiseToken)) == 0) {
	    D(TAG("processCommand", "mIsClockwise was ").c_str()); DL(mIsClockwise ? "CW" : "CCW");
	    DL(TAG("processCommand", "new isClockwise is CCW").c_str());
	    mIsClockwise = false;
	    msgAtMinTicks = cmsgAtDir + strlen(counterclockwiseToken);
	    haveDir = true;
	} else {
	    *msg = cmsgAtDir;
	    StringUtils::consumeToEOL(msg);
	}
    }

    if (haveDir) {
        const char *cmsgAtMinTicks = msgAtMinTicks.c_str();
	D(TAG("processCommand", "have temp and dir; parsing: ").c_str()); DL(cmsgAtMinTicks);
        int newLowerLimitTicks = atoi(cmsgAtMinTicks);
	if (newLowerLimitTicks != mLowerLimitTicks) {
	    D(TAG("processCommand", "mLowerLimitTicks was ").c_str()); DL(mLowerLimitTicks);
	    D(TAG("processCommand", "new lowerLimitTicks is ").c_str()); DL(newLowerLimitTicks);
	    mLowerLimitTicks = newLowerLimitTicks;
	} else {
	    D(TAG("processCommand", "mLowerLimitTicks matches ").c_str()); DL(mLowerLimitTicks);
	}
	Str t(cmsgAtMinTicks);
	StringUtils::consumeNumber(&t);

	if (t.len() && (*t.c_str() == '|')) {
	    const char *cmsgAtMaxTicks = t.c_str() + 1;
	    int newUpperLimitTicks = atoi(cmsgAtMaxTicks);
	    if (newUpperLimitTicks != mUpperLimitTicks) {
	        D(TAG("processCommand", "mUpperLimitTicks was ").c_str()); DL(mUpperLimitTicks);
		D(TAG("processCommand", "new upperLimitTicks is ").c_str()); DL(newUpperLimitTicks);
		mUpperLimitTicks = newUpperLimitTicks;
	    } else {
	        D(TAG("processCommand", "mUpperLimitTicks matches ").c_str()); DL(mUpperLimitTicks);
	    }
	    *msg = cmsgAtMaxTicks;
	    StringUtils::consumeNumber(msg);
	    StringUtils::consumeToEOL(msg);
	} else {
	    *msg = t;
	    StringUtils::consumeToEOL(msg);
	}
    }

    getQueue()->receivedSuccessConfirmation(getFreelist());
}





void ServoConfigActuator::QueueEntry::post(const char *sensorName, Adafruit_BluefruitLE_SPI &ble)
{
    DL("ServoConfigActuator::QueueEntry::post");
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|");
    ble.print(sensorName);
    ble.print("|GETSERVOCONFIG");
    ble.println("\\n");

    // check response status
    if ( ble.waitForOK() ) {
        //DL("got ok after sending GETSERVOCONFIG");
    } else {
        TRACE("ServoConfigActuator::QueueEntry::post; "
	      "didn't get ok after sending GETSERVOCONFIG");
        PL("ServoConfigActuator::QueueEntry::post; "
	   "didn't get ok after sending GETSERVOCONFIG");
    }
}


void ServoConfigActuator::enqueueRequest()
{
    DL("queueing a call for ServoConfigActuator");
    
    ServoConfigActuator::QueueEntry *e = getFreelist()->pop();
    if (e == 0) {
        //DL("nothing popped off freelist; creating a new ServoConfigActuator::QueueEntry");
      e = new ServoConfigActuator::QueueEntry();
    } else {
        //DL("using record from freelist");
    }
		  
	    
    getQueue()->push(e);
}


void ServoConfigActuator::attemptPost(Adafruit_BluefruitLE_SPI &ble)
{
    DL("ServoConfigActuator::attemptPost called");
    getQueue()->attemptPost(ble);
}





