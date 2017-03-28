#include <HumidSensor.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <TempSensor.h>

#include <str.h>

#include <DHT.h>


HumidSensor::HumidSensor(const HiveConfig &config,
			 const char *name,
			 const class RateProvider &rateProvider,
			 unsigned long now, Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, now, wifiMutex), mHumidStr(new Str("NAN"))
{
    enableValueCache(true); // allow the base class to defer POSTs until there's a change in sampled value
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
	int tenths = (int) (10*(t - ((double) degrees)));
	    
	char degreesStr[20];
	sprintf(degreesStr, "%d", degrees);
	    
	char tenthsStr[20];
	sprintf(tenthsStr, "%d", tenths);

	char output[20];
	sprintf(output, "%s.%s", degreesStr, tenthsStr);
	    
	*value = output;
    
	P("Measured humidity: ");
	P(output);
	PL("%");
    }
    return true;
}


