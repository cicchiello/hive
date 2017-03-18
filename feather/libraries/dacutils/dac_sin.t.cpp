#include <dac_sin.t.h>

#include <platformutils.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>



static bool success = true;

const int SAMPLE_RATE = 60000;
const int Amplitude = 368;
const int dcBias = 368+64;
const int minSin = dcBias-Amplitude/2;
const int maxSin = dcBias+Amplitude/2;
const double FREQ = 800;
const double samplesPerCycle = SAMPLE_RATE/FREQ; // samples/s / cycles/s == samples/cycle
int sineSample(long sampleNum, uint16_t min, uint16_t max)
{
    static uint16_t s_samples[((int) samplesPerCycle+10)];
    static bool s_initialized = false;
    static long s_sampleCnt = 0;
    static int s_cycleSample = 0;

    if (!s_initialized) {
        for (int i = 0; i < (int) samplesPerCycle; i++) {
	    float rads = 2.0*PI*((float)i)/samplesPerCycle;
	    double d = sin(rads) + 1.0;

	    uint16_t s = (uint16_t) ((max-min) * (d/2.0)) + min;
	    s_samples[i] = s;
	    //P("Sample["); P(i); P("] = "); PL(s);
	}
	
	s_initialized = true;
    }

    if (s_sampleCnt != sampleNum) {
        //P("Adjusting sample counter; was "); P(s_sampleCnt); P(" should be "); PL(sampleNum);
        int numCycles = (int) (((double)sampleNum) / samplesPerCycle);
	int cycleSample = sampleNum - ((double) numCycles)*samplesPerCycle;
	s_sampleCnt = sampleNum;
	s_cycleSample = cycleSample;
    }
    uint16_t s = s_samples[s_cycleSample++];
    s_sampleCnt++;
    if (s_cycleSample >= (int) samplesPerCycle)
        s_cycleSample = 0;
    return s;
}


static long s_sample = 0;
void DACPulseCallback()
{
    //to see the pulse on the scope, enable "9" as an output and uncomment:
    //const PinDescription &p = g_APinDescription[9];
    //PORT->Group[p.ulPort].OUTTGL.reg = (1ul << p.ulPin) ;

    while (DAC->STATUS.bit.SYNCBUSY == 1)
        ;
    
    DAC->DATA.reg = sineSample(s_sample++, minSin, maxSin);  // DAC on 10 bits.
}


static unsigned long timeToAct = 1500l;
bool DAC_SinTest::setup() {
    WDT_TRACE("DAC_SinTest::setup");
    TF("DAC_SinTest::setup");
    TRACE("entry");
    pinMode(5, OUTPUT);
    pinMode(A0, OUTPUT);
    
    PlatformUtils::nonConstSingleton().initPulseGenerator(0, SAMPLE_RATE);

    // prime the signal generator map
    sineSample(0, minSin, maxSin);  // DAC on 10 bits.
    
    DAC->CTRLA.bit.ENABLE = 0x01;     // Enable DAC
    while (DAC->STATUS.bit.SYNCBUSY == 1)
        ;

    unsigned long now = millis();
    timeToAct += now;

    return success;
}



static bool started = false;
bool DAC_SinTest::loop() {
    TF("DAC_SinTest::loop");
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
        // do it!
	if (now >= timeToAct + 2000l) {
	    TRACE("done");
	    m_didIt = true;
	    PlatformUtils::nonConstSingleton().stopPulseGenerator(0);
	} else {
	    if (!started) {
	        TRACE("started");
		PlatformUtils::nonConstSingleton().startPulseGenerator(0, DACPulseCallback);
		started = true;
	    }
	}
    }

    return success;
}
