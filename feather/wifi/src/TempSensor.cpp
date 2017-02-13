#include <TempSensor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <hiveconfig.h>
#include <http_couchpost.h>

#include <RateProvider.h>
#include <TimeProvider.h>

#include <str.h>

#include <DHT.h>

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN A3       // what pin we're connected to for data

static DHT dht(DHTPIN, DHTTYPE);
static unsigned long lastSampleTime = 0;

static void *MySemaphore = &MySemaphore; // used by mutex



TempSensor::TempSensor(const HiveConfig &config,
		       const char *name,
		       const class RateProvider &rateProvider,
		       const class TimeProvider &timeProvider,
		       unsigned long now)
  : SensorBase(config, name, rateProvider, timeProvider, now),
    mTempStr(new Str("NAN")), mT(25.0), mHasTemp(false)
{
    setNextTime(now, &mNextSampleTime);
    setNextTime(now, &mNextPostTime);
    dht.begin();
}


TempSensor::~TempSensor()
{
    delete mTempStr;
}


/* STATIC */
DHT &TempSensor::getDht()
{
    return dht;
}

bool TempSensor::sensorSample(Str *value)
{
    double t = dht.readTemperature();
    if (isnan(t)) {
        *value = Str("NAN");
	D("Providing stale temp: ");
	DL(value->c_str());
    } else {
        mT = t;
	mHasTemp = true;

	int degrees = (int) t;
	int hundredths = (int) (100*(t - ((double) degrees)));
	    
	char degreesStr[20];
	sprintf(degreesStr, "%d", degrees);
	
	char hundredthsStr[20];
	sprintf(hundredthsStr, "%d", hundredths);

	char output[20];
	sprintf(output, "%s.%s", degreesStr, hundredthsStr);
	    
	*value = output;
    
	D("Measured ambient temp: ");
	DL(output);
    }
}


bool TempSensor::isItTimeYet(unsigned long now)
{
    return (now >= mNextSampleTime) || (now >= mNextPostTime);
}



