#include <Timestamp.h>

#include <Arduino.h>


#define HEADLESS

#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif


#define NDEBUG
#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif

#include <str.h>

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


void Timestamp::QueueEntry::post(Adafruit_BluefruitLE_SPI &ble)
{
    DL("Timestamp::QueueEntry::post");
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|GETTIME");
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
      e = new Timestamp::QueueEntry();
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
    //DL("Timestamp::isTimestampResponse");
    const char *prefix = "rply|GETTIME|";
    return (strncmp(rsp, prefix, strlen(prefix)) == 0);
}


bool Timestamp::processTimestampResponse(const char *rsp)
{
    DL("Timestamp::processTimestampResponse");
    const char *prefix = "rply|GETTIME|";
    char *endPtr;
    mTimestamp = strtol(rsp + strlen(prefix), &endPtr, 10);
    P("Received reply to GETTIME: ");
    PL(mTimestamp);
    bool stat = endPtr > rsp+strlen(prefix);
    mHaveTimestamp = true;
    mSecondsAtMark = (millis()+500)/1000;
    getQueue()->receivedSuccessConfirmation(getFreelist());
    return stat;
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

