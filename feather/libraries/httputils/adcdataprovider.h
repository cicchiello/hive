#ifndef adcdataprovider_h
#define adcdataprovider_h

#include <http_dataprovider.h>

class ADC_BufFifo;

class ADCDataProvider : public HttpDataProvider {
public:
    enum ADCState {Uninitialized, Capturing, Stopped, Done};
    
    ADCDataProvider(unsigned long captureDuration_ms);
    ~ADCDataProvider();

    int takeIfAvailable(unsigned char **buf);
    void giveBack(unsigned char *);

    ADCState getState() const {return m_state;}
    
    void start();
    bool isDone() const;
    int getSize() const;
    void close();

    bool failed() const {return !m_success;}
    
    void bufferReady(unsigned short *buf);
    
    void reset();
    
 private:
    void init();
    
    ADCState m_state;
    bool m_success;
    const unsigned long mCaptureDuration_ms;
    unsigned long m_bufStartTime_us, m_totalSampleTime_us, m_captureStartTime_us;
    int m_bufCnt, mNumBufsToCapture;
    unsigned short *m_bufToPersist;

    ADC_BufFifo *mFifo;
};

#endif
