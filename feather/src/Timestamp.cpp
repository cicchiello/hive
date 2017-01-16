#include <Timestamp.h>

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

#include <str.h>
#include <Parse.h>

#include <txqueue.h>
#include <freelist.h>


static FreeList<Timestamp::QueueEntry>	*s_timestampEntryFreeList = NULL;
static TxQueue<Timestamp::QueueEntry> *s_timestampTxQueue = NULL;


static FreeList<Timestamp::QueueEntry> *getFreelist()
{
    if (s_timestampEntryFreeList == NULL) {
        s_timestampEntryFreeList = new FreeList<Timestamp::QueueEntry>();
    }
    return s_timestampEntryFreeList;
}

static TxQueue<Timestamp::QueueEntry> *getQueue()
{
    if (s_timestampTxQueue == NULL) {
        s_timestampTxQueue = new TxQueue<Timestamp::QueueEntry>();
    }
    return s_timestampTxQueue;
}


Timestamp::Timestamp(const char *resetCause)
  : mRequestedTimestamp(false), mHaveTimestamp(false), mRCause(new Str(resetCause))
{
}


Timestamp::~Timestamp()
{
    delete mRCause;
}


const char *Timestamp::getResetCause() const
{
    return mRCause->c_str();
}


void Timestamp::QueueEntry::post(const char *sensorName, Adafruit_BluefruitLE_SPI &ble)
{
    DL("Timestamp::QueueEntry::post");
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|");
    ble.print(sensorName);
    ble.print("|GETTIME|");
    ble.print(getTimestamp()->getResetCause());
    ble.println("\\n");

    // check response status
    if ( ble.waitForOK() ) {
        //DL("got ok after sending GETTIME");
    } else {
        PL("didn't get ok after sending GETTIME");
    }
}


void Timestamp::enqueueRequest()
{
    DL("queueing a call for timestamp");
    
    Timestamp::QueueEntry *e = getFreelist()->pop();
    if (e == 0) {
        //DL("nothing popped off freelist; creating a new Timestamp::QueueEntry");
      e = new Timestamp::QueueEntry(this);
    } else {
        //DL("using record from freelist");
    }
		  
	    
    getQueue()->push(e);
    mRequestedTimestamp = true;
}


void Timestamp::attemptPost(Adafruit_BluefruitLE_SPI &ble)
{
    getQueue()->attemptPost(ble);
}


bool Timestamp::isTimestampResponse(const char *rsp) const
{
    DL("Timestamp::isTimestampResponse");
    const char *token = "rply|";
    if (strncmp(rsp, token, strlen(token)) == 0) {
        rsp += strlen(token);
	if (strncmp(rsp, getName(), strlen(getName())) == 0) {
	    rsp += strlen(getName());
	    token = "|GETTIME|";
	    return (strncmp(rsp, token, strlen(token)) == 0);
	}
    } 
    return false;
}


const char *Timestamp::processTimestampResponse(const char *rsp)
{
    DL("Timestamp::processTimestampResponse");
    const char *token0 = "rply|";
    const char *token1 = getName();
    const char *token2 = "|GETTIME|";
    Str response(rsp + strlen(token0) + strlen(token1) + strlen(token2));

    char *endPtr;
    mTimestamp = strtol(response.c_str(), &endPtr, 10);
    D("Received reply to GETTIME: ");
    DL(mTimestamp);
    bool stat = endPtr > response.c_str();
    mHaveTimestamp = true;
    mSecondsAtMark = (millis()+500)/1000;
    getQueue()->receivedSuccessConfirmation(getFreelist());
    return Parse::consumeToEOL(endPtr);
}


void Timestamp::toString(unsigned long now, Str *str)
{
    unsigned long secondsSinceBoot = (now+500)/1000;
    unsigned long secondsSinceMark = secondsSinceBoot - secondsAtMark();
    unsigned long secondsSinceEpoch = secondsSinceMark + mTimestamp;

    char timestampStr[16];
    sprintf(timestampStr, "%lu", secondsSinceEpoch);

    *str = timestampStr;
}

