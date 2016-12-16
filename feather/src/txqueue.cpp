#include <txqueue.h>


#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"


#include <cloudpipe.h>
#include <str.h>


#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)


#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#define TIMEOUT_S 5


class Entry {
public:
  Str sensorName, value, timestamp;

  Entry() {}
  
  void set(const char *_sensorName, const char *_value, const char *_timestamp)
  {
    sensorName = _sensorName;
    value = _value;
    timestamp = _timestamp;
  }
  
};


static int len = 0, sz = 10;
static Entry **queue = new Entry*[10];

static int freeLen = 0, freeSz = 10;
static Entry **freeList = new Entry*[10];

static bool queueIsBusy = false;

void TxQueue::push(const char *sensorName, const char *value, const char *timestamp)
{
    if (len>0) {
      D("Queue depth: ");
      DL(len);
      D("queueIsBusy: ");
      DL(queueIsBusy ? "true" : "false");
    }
    
    Entry *e = 0;
    if (freeLen > 0) {
        e = freeList[freeLen-1];
	freeList[freeLen-1] = 0;
	freeLen--;
    } else {
        e = new Entry();
    }
    e->set(sensorName, value, timestamp);
    queue[len++] = e;

    if (len == sz) {
        Entry **newQueue = new Entry*[2*sz];
	for (int i = 0; i < sz; i++) {
	    newQueue[i] = queue[i];
	}
	delete [] queue;
	queue = newQueue;
	sz *= 2;
    }
}


void TxQueue::attemptPost(Adafruit_BluefruitLE_SPI &ble)
{
    static long timeoutExpiry = 0;
    if (queueIsBusy) {
        if (!ble.isConnected())
	    queueIsBusy = false;
	else if (millis() > timeoutExpiry) {
	    queueIsBusy = false;
	}
    } else {
        if ((len>0) && ble.isConnected()) {
            queueIsBusy = true;
	    timeoutExpiry = millis() + TIMEOUT_S*1000;
	    Entry *e = queue[0];
	    D("posting a queued entry with timestamp: ");
	    DL(e->timestamp.c_str());
	    CloudPipe::singleton().uploadSensorReading(ble,
						       e->sensorName.c_str(),
						       e->value.c_str(),
						       e->timestamp.c_str());
	}
    }
}


void TxQueue::receivedFailureConfirmation()
{
    // just need to free up the queue's busy flag, and the next attemp to post
    // will resend the top of the queue
    queueIsBusy = false;
}


void TxQueue::receivedSuccessConfirmation()
{
    // remove the 0th entry from the queue ('cause it's just been confirmed)
    Entry *e = queue[0];

    // bump the queue up one level
    for (int i = 1; i < len; i++) {
        queue[i-1] = queue[i];
    }
    len--;
    
    // put the struct on the freelist
    freeList[freeLen++] = e;
    if (freeLen == freeSz) {
        Entry **newFreeList = new Entry*[2*freeSz];
	for (int i = 0; i < freeSz; i++) {
	    newFreeList[i] = freeList[i];
	}
	delete [] freeList;
	freeList = newFreeList;
	freeSz *= 2;
    }

    queueIsBusy = false;
}

