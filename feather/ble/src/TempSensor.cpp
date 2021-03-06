#include <TempSensor.h>

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
#include <RateProvider.h>

#include <DHT.h>

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN A3       // what pin we're connected to for data

static DHT dht(DHTPIN, DHTTYPE);
static unsigned long lastSampleTime = 0;

TempSensor::TempSensor(const char *name, const class RateProvider &rateProvider, unsigned long now)
  : Sensor(name, rateProvider, now), mPrev(new Str("NAN")), mT(25.0), mHasTemp(false)
{
    dht.begin();
    lastSampleTime = millis();
}


TempSensor::~TempSensor()
{
    delete mPrev;
}


/* STATIC */
DHT &TempSensor::getDht()
{
    return dht;
}

bool TempSensor::sensorSample(Str *value)
{
    unsigned long now = millis();
    if (now - lastSampleTime > 2000) {
	
        double t = dht.readTemperature();
	if (isnan(t)) {
	    *value = *mPrev;
	    D("Providing stale temp: ");
	    DL(value->c_str());
	} else {
	    mT = t;
	    lastSampleTime = now;
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
    } else {
        *value = *mPrev;
	D("Providing stale temp: ");
	DL(value->c_str());
    }
}
