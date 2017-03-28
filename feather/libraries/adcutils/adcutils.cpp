#include <adcutils.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>
#include <adc_bufqueue.h>
#include <platformutils.h>

#include <Arduino.h>



#define DEDICATED_PULSE_GENERATOR 1

static __inline__ void ADCsync() __attribute__((always_inline, unused));
static void   ADCsync() {
    while (ADC->STATUS.bit.SYNCBUSY == 1); //Just wait till the ADC is free
}

/* STATIC */
ADCUtils ADCUtils::s_singleton;
void (*ADCUtils::ADCHandler)() = ADCUtils::Disabled_ADCHandler;


ADCUtils::ADCUtils()
  : m_enabled(false), m_index(0), m_overrunCnt(0), m_conversionsTriggered(0),
    m_bufSz(0), m_persistCb(NULL), m_sampleCnt(0),
    m_freeBufs(NULL), m_currBuf(NULL), m_readyToTrigger(true)
{
    assert(this == &s_singleton, "Singleton rules violated");
    for (int i = 0; i < NumBufs; i++)
        m_bufs[i] = NULL;
    m_freeBufs = new ADC_BufQueue(NumBufs);
}

ADCUtils::~ADCUtils()
{
    for (int i = 0; i < NumBufs; i++)
        delete [] m_bufs[i];
    delete m_freeBufs;
}


void ADCUtils::swapBufs()
{
    assert(m_currBuf, "buf is NULL");
    assert(!m_freeBufs->isEmpty(), "Buffer Queue overflow detected");
    m_persistCb(m_currBuf);
    m_currBuf = m_freeBufs->pop();
    assert(m_currBuf, "buf is NULL");
    assert(!m_freeBufs->isFull(), "Buffer Queue is full even though I just popped");
    m_index = 0;
}


void ADCPulseCallback()
{
    TF("::ADCPulseCallback");
    if (ADCUtils::s_singleton.m_readyToTrigger) {
        ADCsync();
	ADC->SWTRIG.bit.START = 1;
	ADCUtils::s_singleton.m_readyToTrigger = false;
	ADCUtils::s_singleton.m_conversionsTriggered++;

	//to see the pulse on the scope, enable "5" as an output and uncomment:
	//const PinDescription &p = g_APinDescription[5];
	//PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;
    } else {
        TRACE("A sample didn't finish before the next trigger should have happened");
    }
}


void ADCUtils::init(int PIN, size_t bufSz, PersistBufferFunc persistCb)
{
    TF("ADCUtils::init");
    if (!m_enabled) {
        assert(PIN != A0, "A0 should be reserved for DAC");
	assert(g_APinDescription[PIN].ulADCChannelNumber == 3, "Unexpected ADC Channel");

	ADCsync();
	
	// from wiring_analog.c and wiring_private.c
	// pinPeripheral(A2, PIO_ANALOG);
	const PinDescription &pind = g_APinDescription[A2];
	const EPortType &port = pind.ulPort;
	const uint32_t &pin = pind.ulPin;
	uint32_t temp ;
	if ( pin & 1 )
	{
	    // odd pin
	    // Get whole current setup for both odd and even pins and remove odd one
	    temp = (PORT->Group[port].PMUX[pin >> 1].reg) & PORT_PMUX_PMUXE( 0xF ) ;
	    // Set new muxing
	    PORT->Group[port].PMUX[pin >> 1].reg = temp|PORT_PMUX_PMUXO( PIO_ANALOG ) ;
	} else {
	    // even pin
	    temp = (PORT->Group[port].PMUX[pin >> 1].reg) & PORT_PMUX_PMUXO( 0xF ) ;
	    PORT->Group[port].PMUX[pin >> 1].reg = temp|PORT_PMUX_PMUXE( PIO_ANALOG ) ;
	}
	PORT->Group[port].PINCFG[pin].reg |= PORT_PINCFG_PMUXEN ; // Enable port mux
	
	//ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC0_Val; // 1/1.48 VDDANA = 1/1.48* 3V3 = 2.2297
	ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INT1V_Val; // 
	ADCsync();    //  ref 31.6.16

	ADC->INPUTCTRL.bit.MUXPOS = pind.ulADCChannelNumber; // Selection for the positive ADC input
	//ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;
	ADCsync();

	ADC->CTRLA.bit.ENABLE = 0;             // Disable ADC
	ADCsync();

	ADC->INTENCLR.reg = 0xf;
    
	ADC->CTRLB.bit.PRESCALER = 1; //peripheral clock divided by 8
	ADCsync();

	// enable Result Ready
	ADC->INTENSET.bit.RESRDY = 1;
	ADCsync();
    }

    if (m_bufSz != bufSz) {
        for (int i = 0; i < NumBufs; i++) {
	    delete [] m_bufs[i];
	    m_bufs[i] = new uint16_t[bufSz];
	}
    }
    m_bufSz = bufSz;
    m_freeBufs->clear();
    assert(m_freeBufs->isEmpty(), "m_freeBufs->isEmpty()");
    for (int i = 0; i < NumBufs; i++)
        m_freeBufs->push(m_bufs[i]);
    assert(m_freeBufs->isFull(), "Buffer isn't full even though I just filled it");

    m_currBuf = m_freeBufs->pop();
    assert(m_currBuf, "buf is NULL");
    
    assert(!m_freeBufs->isFull(), "Buffer is full even though I just popped");
    m_persistCb = persistCb;
    ADC->CTRLA.bit.ENABLE = 0x01;
    ADCsync();

    ADC->INTFLAG.bit.RESRDY = 1;
    ADCsync();
    
    NVIC_EnableIRQ( ADC_IRQn ) ;

    m_enabled = true;
    m_sampleCnt = 0;
    m_overrunCnt = 0;
    m_conversionsTriggered = 0;
    
    ADCHandler = Enabled_ADCHandler;
    m_readyToTrigger = true;
}


void ADCUtils::init(int PIN, size_t bufSz, int sampleRate, PersistBufferFunc persistCb)
{
    TF("ADCUtils::init (2)");
    init(PIN, bufSz, persistCb);
    
    PlatformUtils::nonConstSingleton().initPulseGenerator(DEDICATED_PULSE_GENERATOR, sampleRate);
    PlatformUtils::nonConstSingleton().startPulseGenerator(DEDICATED_PULSE_GENERATOR, ADCPulseCallback);
}


void ADC_Handler()
{
    ADCUtils::ADCHandler();
}


/* STATIC */
void ADCUtils::Disabled_ADCHandler()
{
    ADC->INTFLAG.reg = ADC_INTFLAG_MASK;
}


/* STATIC */
void ADCUtils::Enabled_ADCHandler()
{
    if (ADC->INTFLAG.bit.RESRDY) {
        assert(s_singleton.m_currBuf, "buf is NULL");
	uint32_t r = ADC->RESULT.bit.RESULT;
	s_singleton.postResult(r);
	assert(!ADC->INTFLAG.bit.RESRDY, "Flag is still set even after reading RESULT!");

	//to see the IRQ call on the scope, enable "6" as an output and uncomment:
	//const PinDescription &p = g_APinDescription[6];
	//PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;

	s_singleton.m_readyToTrigger = true;
    }
    if (ADC->INTFLAG.bit.OVERRUN) {
        s_singleton.m_overrunCnt++;
    }
    ADC->INTFLAG.reg = ADC_INTFLAG_MASK;
}


void ADCUtils::freeBuf(uint16_t *b)
{
    m_freeBufs->push((uint16_t*)b);
}

void ADCUtils::stop()
{
    if (m_currBuf != NULL) {
	PlatformUtils::nonConstSingleton().stopPulseGenerator(DEDICATED_PULSE_GENERATOR);
	ADC->CTRLA.bit.ENABLE = 0x00;
	ADCsync();
	assert(m_currBuf, "buf is NULL");
	assert(!m_freeBufs->isFull(), "m_freeBufs is already full!?!");
	m_freeBufs->push(m_currBuf);
	m_currBuf = NULL;
	m_enabled = false;
	ADCHandler = Disabled_ADCHandler;
    }
}

void ADCUtils::shutdown()
{
    stop();
    assert(m_freeBufs->isFull(), "one or more bufs not freed before shutting down ADC");
}

