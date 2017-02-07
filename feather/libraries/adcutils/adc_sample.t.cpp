#include <adc_sample.t.h>

//#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <platformutils.h>

#include <adcutils.h>

// adcdma
//  analog A5
//   http://www.atmel.com/Images/Atmel-42258-ASF-Manual-SAM-D21_AP-Note_AT07627.pdf pg 73


#define ADCPIN A2
#define HWORDS 512


static ADC_Sample *s_instance = NULL;
#define BYTES_PER_SAMPLE 2


ADC_Sample::ADC_Sample() 
  : m_success(true),
    m_timeToAct(1000000l), m_bufStartTime(0), m_totalSampleTime(0),
    m_bufCnt(0), m_bufToPersist(NULL),
    m_state(Uninitialized)
{
    PF("ADC_Sample::ADC_Sample; ");
    if (s_instance != NULL) {
        PHL("implied singleton rules violated");
	m_success = false;
	s_instance->m_success = false;
    } else {
        s_instance = this;
    }
}

ADC_Sample::~ADC_Sample()
{
    s_instance = NULL;
}

bool ADC_Sample::setup()
{
    unsigned long now = millis();
    m_timeToAct += now;

    return m_success;
}

void ADC_Sample::bufferReady(unsigned short *buf)
{
    TF("ADC_Sample::bufferReady; ");
//    TRACE("entry");
    
    if (m_bufToPersist != NULL) {
        TRACE("called before m_bufToPersist is NULL");
        m_success = false;
    }

    m_bufToPersist = buf;

    unsigned long now = micros();
    m_totalSampleTime += (now - m_bufStartTime);
    m_bufStartTime = now;
}

static void bufferReady(unsigned short *buf)
{
    TF("::bufferReady");
//    TRACE("entry");
    
    if (s_instance == NULL) {
        TRACE("called outside the lifespan of an ADC_Sample object");
    } else {
        s_instance->bufferReady(buf);
    }
}


void ADC_Sample::processBuffer()
{
    PF("ADC_Sample::processBuffer; ");
    
    // have a complete buffer
    m_bufCnt++;
		    
    uint16_t sample0 = m_bufToPersist[0];
    ADCUtils::nonConstSingleton().freeBuf(m_bufToPersist);
    m_bufToPersist = NULL;

    // now there's some time to print stuff before the next buffer is
    // filled -- but only do it once
    if (m_bufCnt == 1) {
        PH(HWORDS); P(" samples taken in ");
	P(m_totalSampleTime);  P(" us; sample[0] is ");
	PL(sample0 >> 6);
    }
}


bool ADC_Sample::loop() {
    TF("ADC_Sample::loop; ");
    unsigned long now = micros();
    if ((now > m_timeToAct) && !m_didIt) {
        switch (m_state) {
	case Uninitialized: {
	    TRACE("Uninitialized");
	    ADCUtils::nonConstSingleton().init(ADCPIN, HWORDS, 22050, ::bufferReady);
	    m_state = Capturing;
	    m_captureStartTime = m_bufStartTime = micros();
	    TRACE("trace");
	}
	    break;
	case Capturing: {
//	    TRACE("Capturing");
	    if (now <= m_captureStartTime + 5000000l) {
	        if (m_bufToPersist != NULL) 
		    // have a complete buffer
		    processBuffer();
	    } else {
	        ADCUtils::nonConstSingleton().stop();
	        m_state = Stopped;
	    }
	}
	    break;
	case Stopped: {
//	    TRACE("Stopped");
	    if (now <= m_captureStartTime + 5500000l) {
	        // waiting in case there's one more buffer being processed
	        if (m_bufToPersist != NULL) 
		    // have a complete buffer
		    processBuffer();
	    } else {
	        ADCUtils::nonConstSingleton().shutdown();
	        PH("Done; ");
		P(m_bufCnt);
		PL(" buffers read");
		P("Samples: ");
		PL(ADCUtils::singleton().numSamples());
		P("Overruns: ");
		PL(ADCUtils::singleton().numOverruns());
		P("Conversions started: ");
		PL(ADCUtils::singleton().numTriggered());
	        P("Done (avg time between batches: ");
		P(((double)m_totalSampleTime) / ((double)m_bufCnt));
		PL(")");
	        m_didIt = true;
	        m_state = Done;
	    }
	}
	  break;
	default: assert(false, "Unexpected state");
	}
    }

    return m_success;
}


