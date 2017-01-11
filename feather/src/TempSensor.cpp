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

#include <DHT.h>

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN A3       // what pin we're connected to for data

static DHT dht(DHTPIN, DHTTYPE);
static unsigned long lastSampleTime = 0;

TempSensor::TempSensor(const char *name, unsigned long now)
  : Sensor(name, now), mPrev(new Str("NAN"))
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
	    P("Providing stale temp: ");
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
    
	    P("Measured ambient temp: ");
	    PL(output);
	}
    } else {
        *value = *mPrev;
	P("Providing stale temp: ");
	PL(value->c_str());
    }
}
