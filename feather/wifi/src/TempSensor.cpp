#include <TempSensor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <str.h>

#include <DHT.h>

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN A3       // what pin we're connected to for data

static DHT dht(DHTPIN, DHTTYPE);


TempSensor::TempSensor(const HiveConfig &config,
		       const char *name,
		       const class RateProvider &rateProvider,
		       const class TimeProvider &timeProvider,
		       unsigned long now, Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex),
    mTempStr(new Str("NAN")), mT(25.0), mHasTemp(false)
{
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

