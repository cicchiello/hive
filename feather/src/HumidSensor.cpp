#include <HumidSensor.h>

#include <Arduino.h>

#include <TempSensor.h>

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

#include <DHT.h>

static unsigned long lastSampleTime = 0;

HumidSensor::HumidSensor(const char *name, const class SensorRateActuator &rateProvider, unsigned long now)
  : Sensor(name, rateProvider, now), mPrev(new Str("NAN"))
{
    lastSampleTime = millis();
    mNextSampleTime += 5*1000; // offset the sample times so that it doesn't come too close to TempSensor's
}


bool HumidSensor::sensorSample(Str *value)
{
    unsigned long now = millis();
    if (now - lastSampleTime > 2000) {
	
        double t = TempSensor::getDht().readHumidity();
	if (isnan(t)) {
	    *value = *mPrev;
	    P("Providing stale humidity: ");
	    PL(value->c_str());
	} else {
	    lastSampleTime = now;
	    
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
    } else {
        *value = *mPrev;
	P("Providing stale humidity: ");
	PL(value->c_str());
    }
}
