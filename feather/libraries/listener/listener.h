#ifndef listener_h
#define listener_h

#include <Arduino.h>

#include <adc_buffifo.h>

class ListenerWavCreator;

class Listener {
public:
    static const uint8_t SAMPLE_RESOLUTION = 10;  // sampling at 10bits
    static const uint32_t BYTES_PER_SAMPLE = 2;
    static const uint32_t SAMPLES_PER_SECOND = 44100;
    static const uint32_t SECONDS_OF_RECORDING = 10;
    static const uint32_t SD_BLK_SZ = 512;

    static const uint16_t SAMPLES_PER_CHUNK = 256;


    Listener(int ADCPIN, int BIASPIN);
    virtual ~Listener();

    bool record(unsigned int duration_ms, const char *filename, bool verbose = false);

    bool loop(bool verbose = false);

    bool isDone() const;

    bool hasError() const;
    const char *getErrmsg() const;

    void fail(const char *errmsg);

 private:
    void processBuffer();
    void writeSD(volatile const uint16_t *buf);
    void flushSD();
    
    void stopRecord();

    const char *m_errMsg, *m_wavFilename;

    // process management
    enum {Uninitialized, Capturing, CaptureStopped, GenWAV, Done} m_state;

    // signal analysis
    int m_max, m_min, m_sampleCnt;
    long m_avg;
    
    // sd-management 
    class SdFat *m_sd;
    class SdFile *m_file;
    uint32_t m_chunkCnt, m_skipCnt;;
    uint16_t m_icache;
    uint8_t *m_cache;

    // adc buffering management
    int m_ADCPIN, m_BIASPIN, m_duration_ms; 
    bool m_initializedADC;
    unsigned long m_endTime, m_captureStartTime_us;
    ADC_BufFifo m_fifo;

    // wav file generation
    ListenerWavCreator *m_wavCreator;
    ListenerWavCreator *initWavFileCreator(const char *rawFilename, const char *wavFilename,
					   int numSamples, int dcBias,
					   uint8_t sampleResolution, uint8_t targetResolution);

    friend void persistBuffer(uint16_t*);
};

inline
bool Listener::isDone() const {
    return hasError() || (m_state == Done);
}

inline
bool Listener::hasError() const
{
    return m_errMsg;
}

inline
const char *Listener::getErrmsg() const
{
    const char *msg = m_errMsg;
    Listener *nonConstThis = (Listener*) this;
    nonConstThis->m_errMsg = NULL;
    return msg;
}

#endif
