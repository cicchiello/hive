#include <HumidSensor.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <hiveconfig.h>
#include <http_couchpost.h>

#include <TempSensor.h>
#include <RateProvider.h>

#include <str.h>

#include <DHT.h>


HumidSensor::HumidSensor(const HiveConfig &config,
			 const char *name,
			 const class RateProvider &rateProvider,
			 const class TimeProvider &timeProvider,
			 unsigned long now, Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex), mHumidStr(new Str("NAN"))
{
}


HumidSensor::~HumidSensor()
{
    delete mHumidStr;
}


bool HumidSensor::sensorSample(Str *value)
{
    double t = TempSensor::getDht().readHumidity();
    if (isnan(t)) {
        *value = *mHumidStr;
	P("Providing stale humidity: ");
	PL(value->c_str());
    } else {
	int degrees = (int) t;
	int hundredths = (int) (100*(t - ((double) degrees)));
	    
	char degreesStr[20];
	sprintf(degreesStr, "%d", degrees);
	    
	char hundredthsStr[20];
	sprintf(hundredthsStr, "%d", hundredths);

	char output[20];
	sprintf(output, "%s.%s", degreesStr, hundredthsStr);
	    
	*value = output;
    
	P("Measured humidity: ");
	P(output);
	PL("%");
    }
    return true;
}


