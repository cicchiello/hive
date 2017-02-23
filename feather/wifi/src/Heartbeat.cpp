#include <Heartbeat.h>
#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <TimeProvider.h>

#include <str.h>


HeartBeat::HeartBeat(const HiveConfig &config,
		     const char *name,
		     const class RateProvider &rateProvider,
		     const class TimeProvider &timeProvider,
		     unsigned long now)
  : SensorBase(config, name, rateProvider, timeProvider, now),
    mCreateTimestampStr(0), mCreateTimestamp(now)
{
    TF("HeartBeat::HeartBeat");
    TRACE2("now: ", now);
    TRACE2("next sample time: ", getNextSampleTime());
}


HeartBeat::~HeartBeat()
{
    delete mCreateTimestampStr;
}


bool HeartBeat::sensorSample(Str *valueStr)
{
    TF("HeartBeat::sensorSample");

    if (mCreateTimestampStr == NULL) {
        if (getTimeProvider().haveTimestamp()) {
	    mCreateTimestampStr = new Str();
	    getTimeProvider().toString(mCreateTimestamp, mCreateTimestampStr);

	    TRACE2("will report uptime as: ", mCreateTimestampStr->c_str());
	    *valueStr = *mCreateTimestampStr;
	} else {
	    *valueStr = "<TBD>";
	}
    } else {
        *valueStr = *mCreateTimestampStr;
    }

    return true;
}



