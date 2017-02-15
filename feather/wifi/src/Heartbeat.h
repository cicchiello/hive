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

    static const int LED_PIN = 13;
    static const unsigned long LED_NORMAL_TOGGLE_RATE_MS = 1000l;  // toggle LED once per second
    static const unsigned long LED_NORMAL_ON_TIME_MS = 50l;        // 1/20 on vs off
    static const unsigned long LED_OFFLINE_TOGGLE_RATE_MS = 3000l; // toggle LED once per 3 seconds
    static const unsigned long LED_OFFLINE_ON_TIME_MS = 1500l;     // 1/1 on vs off
    static const unsigned long LED_ERROR_TOGGLE_RATE_MS = 500l;    // toggle LED twice per second
    static const unsigned long LED_ERROR_ON_TIME_MS = 250l;        // 1/1 on vs off
    
    enum FlashMode {Initial, Normal, Offline, Error};

    FlashMode setFlashMode(FlashMode m);
    
    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now, Mutex *wifi);

    bool sensorSample(Str *value);

 private:
    HeartBeat(const HeartBeat &); // intentionally unimplemented
    const HeartBeat &operator=(const HeartBeat &o); // intentionallly unimplemented
    
    const char *className() const {return "HeartBeat";}

    void considerFlash(unsigned long now, unsigned long times_per_ms, unsigned long onTime_ms);

    Str *mTimestampStr;
    FlashMode mFlashMode;
    bool mLedIsOn, mFlashModeChanged;
    unsigned long mNextActionTime, mNextPostActionTime, mNextBlinkActionTime;
};

inline
HeartBeat::FlashMode HeartBeat::setFlashMode(HeartBeat::FlashMode m)
{
    FlashMode r = mFlashMode;
    mFlashMode = m;
    mFlashModeChanged = true;
    return r;
}

#endif
