#include <SensorRateActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <RateProvider.h>
#include <hiveconfig.h>

#include <strbuf.h>
#include <strutils.h>


static Str SENSOR_RATE_PROPNAME("sensor-rate-seconds");


SensorRateActuator::SensorRateActuator(HiveConfig *config, const char *name, unsigned long now)
  : Actuator(name, now), mConfig(*config),
#ifndef NDEBUG    
    mSeconds(30l)
#else    
    mSeconds(5*60)
#endif  
{
    if (mConfig.hasProperty(SENSOR_RATE_PROPNAME)) {
        const char *value = mConfig.getProperty(SENSOR_RATE_PROPNAME).c_str();
	mSeconds = StringUtils::isNumber(value) ? atoi(value) : mSeconds;
    }
}


bool SensorRateActuator::loop(unsigned long now)
{
    return false;
}


int SensorRateActuator::secondsBetweenSamples() const
{
    TF("SensorRateActuator::secondsBetweenSamples");

    PH2("Reporting secondsBetweenSamples of: ", mSeconds);
    return mSeconds;
}


void SensorRateActuator::processMsg(unsigned long now, const char *msg)
{
    static const char *InvalidMsg = "invalid msg; did isMyMsg return true?";
    
    TF("SensorRateActuator::processMsg");

    const char *name = getName();
    int namelen = strlen(name);
    assert2(strncmp(msg, name, namelen) == 0, InvalidMsg, msg);
    
    msg += namelen;
    const char *token = "|";
    int tokenlen = 1;
    assert2(strncmp(msg, token, tokenlen) == 0, InvalidMsg, msg);
    msg += tokenlen;
    assert2(StringUtils::isNumber(msg), InvalidMsg, msg);

    mSeconds = atoi(msg);
    
    StrBuf secondsStr;
    secondsStr.append(mSeconds);
    PH4("setting HiveConfig property ", SENSOR_RATE_PROPNAME.c_str(), " to ", secondsStr.c_str());
    mConfig.addProperty(SENSOR_RATE_PROPNAME, secondsStr.c_str());
}


bool SensorRateActuator::isMyMsg(const char *msg) const
{
    TF("StepperActuator::isMyMsg");
    const char *name = getName();
    TRACE2("isMyMsg for motor: ", name);
    TRACE2("Considering msg: ", msg);
    
    if (strncmp(msg, name, strlen(name)) == 0) {
        msg += strlen(name);
	const char *token = "|";
	bool r = (strncmp(msg, token, strlen(token)) == 0) && StringUtils::isNumber(msg+strlen(token));
	if (r) {
	    TRACE("it's mine!");
	}
	return r;
    } else {
        return false;
    }
}
