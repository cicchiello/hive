#ifndef adc_sample_h
#define adc_sample_h

#include <tests.h>

class ADC_Sample : public Test{
  public:
    ADC_Sample();
    ~ADC_Sample();

    bool setup();
    bool loop();

    const char *testName() const {return "ADC_Sample";}

    void bufferReady(unsigned short *buf);

 private:
    ADC_Sample(const ADC_Sample&); // intentionally unimplemented

    void processBuffer();
    
    enum {Uninitialized, Capturing, Stopped, Done} m_state;
    
    bool m_success;
    unsigned long m_timeToAct, m_bufStartTime, m_totalSampleTime, m_captureStartTime;
    int m_bufCnt;
    unsigned short *m_bufToPersist;
};

#endif
