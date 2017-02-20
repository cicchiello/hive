#ifndef Indicator_h
#define Indicator_h


class Indicator {
 public:

    Indicator(unsigned long now);
    ~Indicator();

    static const int LED_PIN = 13;
    static const unsigned long LED_NORMAL_TOGGLE_RATE_MS = 1000l;    // toggle LED once per second
    static const unsigned long LED_NORMAL_ON_TIME_MS = 50l;          // 1/20 on vs off
    static const unsigned long LED_TRYING_TOGGLE_RATE_MS = 100l;     // toggle LED fast
    static const unsigned long LED_TRYING_ON_TIME_MS = 50l;          // 1/1 on vs off
    static const unsigned long LED_PROVISION_TOGGLE_RATE_MS = 3000l; // toggle LED once per 3 seconds
    static const unsigned long LED_PROVISION_ON_TIME_MS = 1500l;     // 1/1 on vs off
    static const unsigned long LED_ERROR_TOGGLE_RATE_MS = 500l;      // toggle LED twice per second
    static const unsigned long LED_ERROR_ON_TIME_MS = 250l;          // 1/1 on vs off
    
    enum FlashMode {TryingToConnect, Provisioning, Normal, Error};

    FlashMode setFlashMode(FlashMode m);
    
    bool loop(unsigned long now);

 private:
    Indicator(const Indicator &); // intentionally unimplemented
    const Indicator &operator=(const Indicator &o); // intentionallly unimplemented
    
    const char *className() const {return "Indicator";}

    void considerFlash(unsigned long now, unsigned long times_per_ms, unsigned long onTime_ms);

    FlashMode mFlashMode;
    bool mLedIsOn, mFlashModeChanged;
    unsigned long mNextActionTime;
};

#endif
