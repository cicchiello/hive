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
		     unsigned long now, Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, now, wifiMutex),
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
        if (GetTimeProvider()) {
	    mCreateTimestampStr = new Str();
	    GetTimeProvider()->toString(mCreateTimestamp, mCreateTimestampStr);

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



