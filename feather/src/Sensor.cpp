#include <Sensor.h>

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

#include <RateProvider.h>
#include <str.h>
#include <Parse.h>

#include <cloudpipe.h>

#include <txqueue.h>
#include <freelist.h>


class QueueEntry {
public:
  QueueEntry(const char *sensorName, const char *value, const char *timestamp)
    : mSensorName(sensorName), mTimestamp(timestamp), mValue(value) {}

  void set(const char *sensorName, const char *value, const char *timestamp) {
    mSensorName = sensorName;
    mTimestamp = timestamp;
    mValue = value;
  }
  
  void post(const char *sensorName, class Adafruit_BluefruitLE_SPI &ble);

  const char *getName() const {return mSensorName.c_str();}
  
private:
  Str mSensorName, mTimestamp, mValue;
};
  

static FreeList<QueueEntry> *s_s_sensorEntryFreeList = NULL;
static TxQueue<QueueEntry> *s_txQueue = NULL;


static FreeList<QueueEntry> *getFreelist()
{
    if (s_s_sensorEntryFreeList == NULL) {
        s_s_sensorEntryFreeList = new FreeList<QueueEntry>();
    }
    return s_s_sensorEntryFreeList;
}

static TxQueue<QueueEntry> *getQueue()
{
    if (s_txQueue == NULL) {
        s_txQueue = new TxQueue<QueueEntry>();
    }
    return s_txQueue;
}



Sensor::Sensor(const char *sensorName, const RateProvider &rateProvider, unsigned long now)
  : mRateProvider(rateProvider)
{
    mName = new Str(sensorName);
    
    // schedule first sample time
    mNextSampleTime = now + 20*1000;
}


Sensor::~Sensor()
{
    delete mName;
}

const char *Sensor::getName() const
{
    return mName->c_str();
}
    
bool Sensor::isItTimeYet(unsigned long now)
{
    return now >= mNextSampleTime;
}


void Sensor::scheduleNextSample(unsigned long now)
{
    unsigned long deltaSeconds = mRateProvider.secondsBetweenSamples();
    
    D("Sensor::scheduleNextSample(");
    D(getName());
    D("); scheduling next sample in ");
    D(deltaSeconds);
    DL(" seconds");
    
    mNextSampleTime = now + deltaSeconds*1000;
}


bool Sensor::isMyResponse(const char *rsp) const
{
    DL("Sensor::isMyResponse");
    const char *token = "rply|";
    if (strncmp(rsp, token, strlen(token)) == 0) {
        rsp += strlen(token);
	if (strncmp(rsp, mName->c_str(), mName->len()) == 0) {
	    rsp += mName->len();
	    token = "|POST|";
	    return (strncmp(rsp, token, strlen(token)) == 0);
	}
    } 
    return false;
}


void Sensor::enqueueRequest(const char *value, const char *timestamp)
{
    enqueueFullRequest(mName->c_str(), value, timestamp);
}


void Sensor::enqueueFullRequest(const char *sensorName, const char *value, const char *timestamp)
{
    QueueEntry *e = getFreelist()->pop();
    if (e == 0) e = new QueueEntry(sensorName, value, timestamp);
    else e->set(sensorName, value, timestamp);

    getQueue()->push(e);
}


void Sensor::attemptPost(Adafruit_BluefruitLE_SPI &ble)
{
    getQueue()->attemptPost(ble);
}


const char *Sensor::processResponse(const char *rsp)
{
    const char *token0 = "rply|";
    const char *token1 = mName->c_str();
    const char *token2 = "|POST|";
    Str response(rsp + strlen(token0) + strlen(token1) + strlen(token2));

    D("Received reply to POST: ");
    DL(response.c_str());

    static const char *success = "success";
    if (strncmp(response.c_str(), success, strlen(success)) == 0) {
        getQueue()->receivedSuccessConfirmation(getFreelist());

	return Parse::consumeEOL(response.c_str() + strlen(success));
    } else {
        static const char *err = "error";
	if (strncmp(response.c_str(), err, strlen(err)) == 0) {
	    getQueue()->receivedFailureConfirmation();

	    P("Received reply to POST: "); PL(response.c_str());
	    PL("Treating as error and consuming to EOL");
	    return Parse::consumeToEOL(response.c_str() + strlen(err));
	} else {
	    getQueue()->receivedFailureConfirmation();

	    return Parse::consumeEOL(response.c_str());
	}
    }
}



void QueueEntry::post(const char *sensorName, Adafruit_BluefruitLE_SPI &ble)
{
    Str macAddress;
    CloudPipe::singleton().getMacAddress(&macAddress);
    
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|");
    ble.print(sensorName);
    ble.print("|POST|");
    ble.print(CloudPipe::SensorLogDb);
    ble.print("|");
    ble.print("{\"hiveid\":\"");
    ble.print(macAddress.c_str());
    ble.print("\",\"sensor\":\"");
    ble.print(mSensorName.c_str());
    ble.print("\",\"timestamp\":\"");
    ble.print(mTimestamp.c_str());
    ble.print("\",\"value\":\"");
    ble.print(mValue.c_str());
    ble.print("\"}");
    ble.println("\\n");

    // check response status
    if ( ble.waitForOK() ) {
    } else {
        PL(F("Failed to send sensor message"));
    }
}


