#include <adcdataprovider.h>

#include <Arduino.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <str.h>

#include <adc_buffifo.h>
#include <adcutils.h>

#define ADCPIN                A2
#define SAMPLES_PER_BUFFER   512
#define SAMPLE_RATE        22050


static ADCDataProvider *s_instance = NULL;
#define BYTES_PER_SAMPLE 2


static void bufferReady(unsigned short *buf)
{
    TF("::bufferReady");
    // have a complete buffer
		    
    if (s_instance == NULL) {
        TRACE("called outside the lifespan of an ADCDataProvider object");
    } else {
        s_instance->bufferReady(buf);
    }
}


void ADCDataProvider::init()
{
    TF("ADCDataProvider::init");
    m_state = Uninitialized;
    m_bufStartTime_us = m_totalSampleTime_us = m_captureStartTime_us = 0;
    m_bufCnt = 0;
    m_bufToPersist = 0;
    m_success = true;
    
    mNumBufsToCapture = (mCaptureDuration_ms*SAMPLE_RATE)/1000l/SAMPLES_PER_BUFFER;
    TRACE3("setup to capture ", mNumBufsToCapture, " buffers");
    TRACE3("@ ", SAMPLES_PER_BUFFER, " samples per buffer");
}


ADCDataProvider::ADCDataProvider(unsigned long captureDuration_ms)
  : mCaptureDuration_ms(captureDuration_ms),
    mFifo(new ADC_BufFifo(4))
{
    TF("ADCDataProvider::ADCDataProvider");
    TRACE("entry");
    assert(s_instance == NULL, "implied singleton rule violated");
    s_instance = this;

    init();
}


void ADCDataProvider::reset()
{
    init();
}


ADCDataProvider::~ADCDataProvider()
{
    s_instance = NULL;
    delete mFifo;
}


void ADCDataProvider::start()
{
    ADCUtils::nonConstSingleton().init(ADCPIN, SAMPLES_PER_BUFFER, 22050, ::bufferReady);
    m_state = Capturing;
    m_captureStartTime_us = m_bufStartTime_us = micros();
}


int ADCDataProvider::takeIfAvailable(unsigned char **buf)
{
    int l = mFifo->isEmpty() ? 0 : SAMPLES_PER_BUFFER*BYTES_PER_SAMPLE;
    if (l > 0) 
        *buf = (unsigned char*) mFifo->pop();
    return l;
}

void ADCDataProvider::giveBack(unsigned char *buf)
{
//    TF("ADCDataProvider::giveBack");
//    TRACE("gave a buffer back");

    // give the buffer back to the ISR
    ADCUtils::nonConstSingleton().freeBuf((unsigned short*) buf);
    
//    uint16_t sample0 = m_bufToPersist[0];
//    ADCUtils::nonConstSingleton().freeBuf(m_bufToPersist);
//    m_bufToPersist = NULL;
//    PL("-");

#if 0    
    // now there's some time to print stuff before the next buffer is
    // filled -- but only do it once
    if (m_bufCnt == 1) {
        TRACE(Str("There were ").
	         append(SAMPLES_PER_BUFFER).
	         append(" samples taken in ").
	         append(m_totalSampleTime_us).
	         append(" us").c_str());
	TRACE(Str("sample[0] is ").
	         append(sample0 >> 6).c_str());
    }
#endif    
}


bool ADCDataProvider::isDone() const
{
    return mNumBufsToCapture == m_bufCnt;
}


int ADCDataProvider::getSize() const
{
    return mNumBufsToCapture*SAMPLES_PER_BUFFER*BYTES_PER_SAMPLE;
}


void ADCDataProvider::close()
{
    TF("ADCDataProvider::close");
    
    ADCUtils::nonConstSingleton().stop();
    ADCUtils::nonConstSingleton().shutdown();
    
    TRACE3("Done; ", m_bufCnt, " buffers read");
    TRACE2("Samples: ", ADCUtils::singleton().numSamples());
    TRACE2("Overruns: ", ADCUtils::singleton().numOverruns());
    //TRACE2("Conversions started: ", ADCUtils::singleton().numTriggered());
    TRACE3("Done (avg time between batches: ", 
	   (((double)m_totalSampleTime_us) / ((double)m_bufCnt)),
	   " us");
}


void ADCDataProvider::bufferReady(unsigned short *buf)
{
    // have a complete buffer, set it aside until the stream can consume it
//    PL("+");

    if (mFifo->isFull()) {
        TF("ADCDataProvider::bufferReady");
	TRACE("Fifo is full!?!?");
	m_success = false;
    } else {
        mFifo->push(buf);
    }
    
//    if (m_bufToPersist != NULL) {
//        TF("ADCDataProvider::bufferReady");
//        TRACE("called before m_bufToPersist is NULL");
//        m_success = false;
//    }

//    m_bufToPersist = buf;

    // tally the buffer's arrival
    unsigned long now = micros();
    m_totalSampleTime_us += (now - m_bufStartTime_us);
    m_bufStartTime_us = now;
    m_bufCnt++;
}
