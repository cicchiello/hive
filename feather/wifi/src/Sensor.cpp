#include <Sensor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>


#include <RateProvider.h>
#include <TimeProvider.h>

#include <str.h>
#include <strbuf.h>
#include <strutils.h>


Sensor::Sensor(const char *sensorName,
	       const RateProvider &rateProvider,
	       const TimeProvider &timeProvider,
	       unsigned long now)
  : mRateProvider(rateProvider), mTimeProvider(timeProvider)
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
    

StrBuf Sensor::TAG(const char *memberfunc, const char *msg) const
{
    StrBuf func(className());
    func.append("(");
    func.append(mName->c_str());
    func.append(")::");
    func.append(memberfunc);
    return StringUtils::TAG(func.c_str(), msg);
}
