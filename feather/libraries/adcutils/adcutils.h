#ifndef adcutils_h
#define adcutils_h

#include <Arduino.h>

typedef void (*PersistBufferFunc)(uint16_t *buffer);

class ADCUtils {
public:
    static const ADCUtils &singleton();
    static ADCUtils &nonConstSingleton();

    static void (*ADCHandler)();
    
    void init(int PIN, size_t bufSz, int sampleRate, PersistBufferFunc persistCB);

    void freeBuf(uint16_t *buffer);

    void stop(); // call as soon as you want the capture process to stop

    void shutdown(); // call after you've finished client cleanups (like calls to freeBuf);

    int numSamples() const;
    int numOverruns() const;
    int numTriggered() const;
    int numSamplesPerBuf() const;

    static void Disabled_ADCHandler();
    static void Enabled_ADCHandler();
    
 private:
    static const uint8_t NumBufs = 10;
    
    static ADCUtils s_singleton;
    ADCUtils();
    ~ADCUtils();

    void postResult(uint16_t r);
    void swapBufs();
    
    int m_sampleCnt, m_index, m_overrunCnt, m_conversionsTriggered, m_bufSz;

    uint16_t *m_bufs[NumBufs], *m_currBuf;
    class ADC_BufQueue *m_freeBufs;
    PersistBufferFunc m_persistCb;
    bool m_enabled, m_readyToTrigger;

    friend void ADCPulseCallback();
};

inline
const ADCUtils &ADCUtils::singleton()
{
    return s_singleton;
}

inline
ADCUtils &ADCUtils::nonConstSingleton()
{
    return s_singleton;
}

inline
int ADCUtils::numSamples() const
{
    return m_sampleCnt;
}

inline
int ADCUtils::numSamplesPerBuf() const
{
    return m_bufSz;
}

inline
int ADCUtils::numOverruns() const
{
    return m_overrunCnt;
}

inline
int ADCUtils::numTriggered() const
{
    return m_conversionsTriggered;
}

inline
void ADCUtils::postResult(uint16_t r)
{
//    if (m_currBuf != NULL) {
//      m_sampleCnt++;
	m_currBuf[m_index++] = r;
	if (m_index == m_bufSz) {
	    m_sampleCnt += m_bufSz;
	    swapBufs();
	}
//    }
}

#endif
