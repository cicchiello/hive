#include <SensorRateActuator.h>

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

#include <platformutils.h>

#include <str.h>
#include <strutils.h>


#include <txqueue.h>
#include <freelist.h>


static FreeList<SensorRateActuator::QueueEntry>	*s_rateRequestEntryFreeList = NULL;
static TxQueue<SensorRateActuator::QueueEntry> *s_rateRequestTxQueue = NULL;


static FreeList<SensorRateActuator::QueueEntry> *getFreelist()
{
    if (s_rateRequestEntryFreeList == NULL) {
        s_rateRequestEntryFreeList = new FreeList<SensorRateActuator::QueueEntry>();
    }
    return s_rateRequestEntryFreeList;
}

static TxQueue<SensorRateActuator::QueueEntry> *getQueue()
{
    if (s_rateRequestTxQueue == NULL) {
        s_rateRequestTxQueue = new TxQueue<SensorRateActuator::QueueEntry>();
    }
    return s_rateRequestTxQueue;
}




SensorRateActuator::SensorRateActuator(const char *name, unsigned long now)
  : Actuator(name, now+10*1000), mSeconds(5*60)
{
    // schedule first sample time
//    mNextActionTime = now + 7*1000; // base class will set to 5s; use 7s to have this running off sync of others
}

SensorRateActuator::~SensorRateActuator()
{
}

void SensorRateActuator::act(class Adafruit_BluefruitLE_SPI &ble)
{
    DL("SensorRateActuator::act called");
    if (getQueue()->getLen() == 0) {
        enqueueRequest();
    }
    attemptPost(ble);
}


bool SensorRateActuator::isItTimeYet(unsigned long now) const
{
    return now >= mNextActionTime;
}


void SensorRateActuator::scheduleNextAction(unsigned long now)
{
    unsigned long deltaSeconds = 60;
    mNextActionTime = now + deltaSeconds*1000;
}


bool SensorRateActuator::isMyCommand(const Str &msg) const
{
    //DL("SensorRateActuator::isMyResponse");
    const char *token = "action|";
    const char *cmsg = msg.c_str();
    
    if (strncmp(cmsg, token, strlen(token)) == 0) {
        cmsg += strlen(token);
	const char *name = getName();
	if (strncmp(cmsg, name, strlen(name)) == 0) {
	    cmsg += strlen(name);
	    
	    token = "|";
	    bool r = (strncmp(cmsg, token, strlen(token)) == 0);
	    if (r) {
	        D("SensorRateActuator::isMyResponse(");
		D(getName());
		DL("); claiming a response");
	    }
	    return r;
	}
    } 
    return false;
}


void SensorRateActuator::processCommand(Str *msg)
{
    const char *token0 = "action|";
    const char *token1 = getName();
    const char *token2 = "|";
    const char *cmsg = msg->c_str() + strlen(token0) + strlen(token1) + strlen(token2);

    D(TAG("processCommand", "parsing: ").c_str()); DL(cmsg);
    int newRate = atoi(cmsg);
    newRate *= 60;
    if (newRate != mSeconds) {
        D(TAG("processCommand", "mSeconds was ").c_str()); DL(mSeconds);
	D(TAG("processCommand", "new rate is ").c_str()); DL(newRate);
        mSeconds = newRate;
    } else {
        DL(TAG("processCommand", "mSeconds left unchanged").c_str());
    }

    *msg = cmsg;
    StringUtils::consumeToEOL(msg);

    if (msg->len()) {
        D(TAG("processCommand", "Here's what's left of the line: ").c_str()); DL(msg->c_str());
    }

    getQueue()->receivedSuccessConfirmation(getFreelist());
}





void SensorRateActuator::QueueEntry::post(const char *sensorName, Adafruit_BluefruitLE_SPI &ble)
{
    DL("SensorRateActuator::QueueEntry::post");
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|");
    ble.print(sensorName);
    ble.print("|GETSAMPLERATE");
    ble.println("\\n");

    // check response status
    if ( ble.waitForOK() ) {
        //DL("got ok after sending GETSAMPLERATE");
    } else {
        PL("didn't get ok after sending GETSAMPLERATE");
    }
}


void SensorRateActuator::enqueueRequest()
{
    DL("queueing a call for SensorRate");
    
    SensorRateActuator::QueueEntry *e = getFreelist()->pop();
    if (e == 0) {
        //DL("nothing popped off freelist; creating a new SensorRateActuator::QueueEntry");
      e = new SensorRateActuator::QueueEntry();
    } else {
        //DL("using record from freelist");
    }
		  
	    
    getQueue()->push(e);
}


void SensorRateActuator::attemptPost(Adafruit_BluefruitLE_SPI &ble)
{
    DL("SensorRateActuator::attemptPost called");
    getQueue()->attemptPost(ble);
}





