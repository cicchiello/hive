#include <Sensor.h>

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
  
  void post(class Adafruit_BluefruitLE_SPI &ble);

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



Sensor::Sensor(unsigned long now)
{
    // schedule first sample time
    mNextSampleTime = now + 20*1000;
}


bool Sensor::isItTimeYet(unsigned long now)
{
    return now >= mNextSampleTime;
}


void Sensor::scheduleNextSample(unsigned long now)
{
    mNextSampleTime = now + 60*1000;
}


bool Sensor::isMyResponse(const char *rsp) const
{
    DL("Sensor::isMyResponse");
    const char *prefix = "rply|POST|";
    return (strncmp(rsp, prefix, strlen(prefix)) == 0);
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


static char *consumeToEOL(const char *rsp)
{
    char *c = (char*) rsp;
    if (*c == '\n' || *c == '\l')
        c++;
    if (*c == '\\' && *(c+1) == 'n')
        c += 2;
    return c;
}


char *Sensor::processResponse(const char *rsp)
{
    const char *prefix = "rply|POST|";
    Str response(rsp + strlen(prefix));
    D("Received reply to POST: ");
    DL(response.c_str());

    static const char *success = "success";
    if (strncmp(response.c_str(), success, strlen(success)) == 0) {
        getQueue()->receivedSuccessConfirmation(getFreelist());

	return consumeToEOL(response.c_str() + strlen(success));
    } else {
        getQueue()->receivedFailureConfirmation();

	return consumeToEOL(response.c_str());
    }
}



void QueueEntry::post(Adafruit_BluefruitLE_SPI &ble)
{
    Str macAddress;
    CloudPipe::singleton().getMacAddress(&macAddress);
    
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|POST|");
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


