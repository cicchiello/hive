#ifndef platform_utils_h
#define platform_utils_h

#include <str.h>

class PlatformUtils {
  public:
    static const PlatformUtils &singleton();
    static PlatformUtils &nonConstSingleton();

    const char *serialNumber() const;

    // full system reset, ensuring that the Feather's bootloader is launched at reset
    void resetToBootloader();

    // full system reset without attempting to re-enter bootloader
    void reset();

    
    void setRTC(const char *timestr);

    
    //---------------------------------------------------------------------------
    // Pulse Generator routines
    //
    typedef void (*PulseCallbackFunc)();
    const int NumPulseGenerators = 2;
    void initPulseGenerator(int whichGenerator, int pulsesPerSecond);
    void startPulseGenerator(int whichGenerator, PulseCallbackFunc cb);
    void stopPulseGenerator(int whichGenerator);
    //
    //---------------------------------------------------------------------------

    

    //---------------------------------------------------------------------------
    // WDT routines
    //
    static const long WDT_MaxTime_ms = 5000l;
    
    // returns whatever func is already registered, then replaces it with the new one before
    // enabling the WDT
    typedef void (*WDT_EarlyWarning_Func)();
    WDT_EarlyWarning_Func initWDT(WDT_EarlyWarning_Func trigger);

    // shuts down the WDT and replaces the the WDT trigger function with the given one
    void shutdownWDT(WDT_EarlyWarning_Func replacement);

    static const char *s_traceStr;
    static unsigned long s_traceTime;
    
    void clearWDT();
    void markWDT(const char *msg, unsigned long now) const;

    long getCountdownVal() const;
    void setCountdownVal(long v);
    //
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    //
    static const char *resetCause();
    
    //
    //---------------------------------------------------------------------------

 private:
    static PlatformUtils s_singleton;
    PlatformUtils();

    long m_wdt_countdown;
    bool m_pulseGeneratorInitialized, m_hardwareInitialized;
    Str m_timestamp;
};


extern void WDT_TRACE(const char *msg);


inline
const PlatformUtils &PlatformUtils::singleton()
{
    return s_singleton;
}

inline
PlatformUtils &PlatformUtils::nonConstSingleton()
{
    return s_singleton;
}

inline
long PlatformUtils::getCountdownVal() const
{
    return m_wdt_countdown;
}

inline
void PlatformUtils::setCountdownVal(long v)
{
    m_wdt_countdown = v;
}

inline
void PlatformUtils::setRTC(const char *timestamp)
{
    m_timestamp = timestamp;
}

inline
void PlatformUtils::markWDT(const char *msg, unsigned long now) const
{
    s_traceStr = msg;
    s_traceTime = now;
}

#endif
