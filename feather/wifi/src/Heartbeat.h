#ifndef HeartBeat_h
#define HeartBeat_h


#include <SensorBase.h>

class HiveConfig;

class HeartBeat : public SensorBase {
 public:

    HeartBeat(const HiveConfig &config,
	      const char *name,
	      const class RateProvider &rateProvider,
	      const class TimeProvider &timeProvider,
	      unsigned long now);
    ~HeartBeat();

    bool sensorSample(Str *value);

 private:
    const char *className() const {return "HeartBeat";}

    Str *mCreateTimestampStr;
    unsigned long mCreateTimestamp;
};

#endif
