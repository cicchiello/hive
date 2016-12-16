#include <platformutils.h>

#include <Arduino.h>

//#define NDEBUG
#include <strutils.h>


#define EARLY_WARNING_TIME_MS 61l

#define nibHigh(b) (((b)&0xf0)>>4)
#define nibLow(b) ((b)&0x0f)

inline static char hex2asc(unsigned char n) 
{
    return n>9 ? n-10+'a' : n+'0';
}

/* STATIC */
PlatformUtils PlatformUtils::s_singleton;



// buf must be 17 bytes
static const char *serialId(char *buf)
{
    volatile uint8_t *p1 = (volatile uint8_t *)0x0080A00C;
    buf[0] = hex2asc(nibHigh(*p1));
    buf[1] = hex2asc(nibLow(*p1++));
    buf[2] = hex2asc(nibHigh(*p1));
    buf[3] = hex2asc(nibLow(*p1++));
    buf[4] = hex2asc(nibHigh(*p1));
    buf[5] = hex2asc(nibLow(*p1++));
    buf[6] = hex2asc(nibHigh(*p1));
    buf[7] = hex2asc(nibLow(*p1++));
    
    p1 = (volatile uint8_t *)0x0080A040;
    buf[8] = hex2asc(nibHigh(*p1));
    buf[9] = hex2asc(nibLow(*p1++));
    buf[10] = hex2asc(nibHigh(*p1));
    buf[11] = hex2asc(nibLow(*p1++));
    buf[12] = hex2asc(nibHigh(*p1));
    buf[13] = hex2asc(nibLow(*p1++));
    buf[14] = hex2asc(nibHigh(*p1));
    buf[15] = hex2asc(nibLow(*p1++));

    buf[16] = hex2asc(nibHigh(*p1));
    buf[17] = hex2asc(nibLow(*p1++));
    buf[18] = hex2asc(nibHigh(*p1));
    buf[19] = hex2asc(nibLow(*p1++));
    buf[20] = hex2asc(nibHigh(*p1));
    buf[21] = hex2asc(nibLow(*p1++));
    buf[22] = hex2asc(nibHigh(*p1));
    buf[23] = hex2asc(nibLow(*p1++));

    buf[24] = hex2asc(nibHigh(*p1));
    buf[25] = hex2asc(nibLow(*p1++));
    buf[26] = hex2asc(nibHigh(*p1));
    buf[27] = hex2asc(nibLow(*p1++));
    buf[28] = hex2asc(nibHigh(*p1));
    buf[29] = hex2asc(nibLow(*p1++));
    buf[30] = hex2asc(nibHigh(*p1));
    buf[31] = hex2asc(nibLow(*p1++));
}


const char *PlatformUtils::serialNumber() const
{
    static bool s_serialInitialized = false;
    static char s_serialBuf[17];
    if (!s_serialInitialized) {
        serialId(s_serialBuf);
    }

    return s_serialBuf;
}


PlatformUtils::PlatformUtils()
  : m_wdt_countdown(PlatformUtils::WDT_MaxTime_ms/EARLY_WARNING_TIME_MS),
    m_pulseGeneratorInitialized(false)
{
}


static void noopPulseHandler()
{
}


static PlatformUtils::PulseCallbackFunc s_pulseCb[] = {noopPulseHandler, noopPulseHandler};


static void choosePrescale(uint8_t *prescaler, unsigned int *prescale, int pulsesPerSecond)
{
    PF("::choosePrescale; ");
    *prescaler = TC_CTRLA_PRESCALER_DIV1_Val;
    *prescale = 1;
    while ((*prescaler < TC_CTRLA_PRESCALER_DIV1024_Val) &&
	   (SystemCoreClock / *prescale / pulsesPerSecond > 0xffff)) {
	switch (++(*prescaler)) {
	case TC_CTRLA_PRESCALER_DIV2_Val: *prescale = 2; break;
	case TC_CTRLA_PRESCALER_DIV4_Val: *prescale = 4; break;
	case TC_CTRLA_PRESCALER_DIV8_Val: *prescale = 8; break;
	case TC_CTRLA_PRESCALER_DIV16_Val: *prescale = 16; break;
	case TC_CTRLA_PRESCALER_DIV64_Val: *prescale = 64; break;
	case TC_CTRLA_PRESCALER_DIV256_Val: *prescale = 256; break;
	case TC_CTRLA_PRESCALER_DIV1024_Val: *prescale = 1024; break;
	default:
	  DHL("Overflow in prescaler loop");
	}
    }
    DH("Choosing prescaler of ");
    DL(*prescale);
}

void PlatformUtils::initPulseGenerator(int whichGenerator, int pulsesPerSecond)
{
    PF("PlatformUtils::initPulseGenerator; ");
    if (!m_pulseGeneratorInitialized) {
        // Enable GCLK for TC4 and TC5 (timer counter input clock)
        GCLK->CLKCTRL.reg =
	    (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5));
	while (GCLK->STATUS.bit.SYNCBUSY)
	    ; //Just wait till the GCLK is available
	
	m_pulseGeneratorInitialized = true;
    }

    switch (whichGenerator) {
    case 0: {
        TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
	while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
	while (TC5->COUNT16.CTRLA.bit.SWRST)
	    ;

	// Set Timer counter Mode to 16 bits
	TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
	// Set TC5 mode as match frequency
	TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

	uint8_t prescaler;
	unsigned int prescale;
	choosePrescale(&prescaler, &prescale, pulsesPerSecond);
	
	//set prescaler and enable TC5
	TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER(prescaler) | TC_CTRLA_ENABLE;
	
	while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;

	// Configure interrupt request
	NVIC_DisableIRQ(TC5_IRQn);
	NVIC_ClearPendingIRQ(TC5_IRQn);
	NVIC_SetPriority(TC5_IRQn, 0);
	NVIC_EnableIRQ(TC5_IRQn);
	
	TC5->COUNT16.INTENSET.bit.MC0 = 1;
	while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
	
	//set TC5 timer counter based off of the system clock and the user defined sample rate
	TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / prescale / pulsesPerSecond - 1);
	while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;

	TC5->COUNT16.INTENSET.bit.MC0 = 1;
	while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
    }
      break;
    case 1: {
        TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
	while (TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
	while (TC4->COUNT16.CTRLA.bit.SWRST)
	    ;

	// Set Timer counter Mode to 16 bits
	TC4->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
	// Set TC4 mode as match frequency
	TC4->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

	uint8_t prescaler;
	unsigned int prescale;
	choosePrescale(&prescaler, &prescale, pulsesPerSecond);
	
	//set prescaler and enable TC4
	TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER(prescaler) | TC_CTRLA_ENABLE;
	
	while (TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;

	// Configure interrupt request
	NVIC_DisableIRQ(TC4_IRQn);
	NVIC_ClearPendingIRQ(TC4_IRQn);
	NVIC_SetPriority(TC4_IRQn, 0);
	NVIC_EnableIRQ(TC4_IRQn);
	
	TC4->COUNT16.INTENSET.bit.MC0 = 1;
	while (TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
	
	//set TC4 timer counter based off of the system clock and the user defined sample rate
	TC4->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / prescale / pulsesPerSecond - 1);
	while (TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;

	TC4->COUNT16.INTENSET.bit.MC0 = 1;
	while (TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
    }
      break;
    }
}


void TC5_Handler (void)
{
    s_pulseCb[0]();
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
    while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
      ;
}


void TC4_Handler (void)
{
    s_pulseCb[1]();
    TC4->COUNT16.INTFLAG.bit.MC0 = 1;
    while (TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
      ;
}


//This function enables TC5 and waits for it to be ready
void PlatformUtils::startPulseGenerator(int whichGenerator, PlatformUtils::PulseCallbackFunc cb)
{
    switch (whichGenerator) {
    case 0:
        if (cb == NULL) s_pulseCb[0] = noopPulseHandler;
	else s_pulseCb[0] = cb;
        TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
	while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
	break;
    case 1:
        if (cb == NULL) s_pulseCb[1] = noopPulseHandler;
	else s_pulseCb[1] = cb;
        TC4->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
	while (TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
	    ;
	break;
    }
}


void PlatformUtils::stopPulseGenerator(int whichGenerator)
{
    s_pulseCb[whichGenerator] = noopPulseHandler;
    
    TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
      ;
}



#define BOOT_DOUBLE_TAP_ADDRESS           (0x20007FFCul)
#define BOOT_DOUBLE_TAP_DATA              (*((volatile uint32_t *) BOOT_DOUBLE_TAP_ADDRESS))
#define DOUBLE_TAP_MAGIC 0x07738135
void PlatformUtils::resetToBootloader()
{
    // by setting this magic number, I'll be indicating to the bootloader that a double-tap
    // was already started -- so it'll move right to bootloader mode on the reset the follows
    BOOT_DOUBLE_TAP_DATA = DOUBLE_TAP_MAGIC;

    // force an immediate system reset 
    NVIC_SystemReset();
}



static __inline__ void WDTsync() __attribute__((always_inline, unused));
static void WDTsync() {
    while (WDT->STATUS.bit.SYNCBUSY)
      ;
}

static void (*s_WDT_EarlyWarning_Func)() = NULL;


PlatformUtils::WDT_EarlyWarning_Func PlatformUtils::initWDT(PlatformUtils::WDT_EarlyWarning_Func handler)
{
    WDT_EarlyWarning_Func oldFunc = s_WDT_EarlyWarning_Func;
    if ((oldFunc == NULL) && (handler != NULL)) {
        // first handler being installed -- so full initialize
        WDT->CONFIG.bit.PER = WDT_CONFIG_PER_9_Val; // 4096 clock cycles (~4s)
	WDT->EWCTRL.bit.EWOFFSET = WDT_EWCTRL_EWOFFSET_8_Val; // 2048 clock cycles (~2s)
	WDTsync();
	WDT->INTENSET.bit.EW = 1;
	WDT->CTRL.bit.ALWAYSON = 0;
	WDT->CTRL.bit.ENABLE = 1;
	WDTsync();

	NVIC_EnableIRQ(WDT_IRQn);
    }
    
    s_WDT_EarlyWarning_Func = handler;
    
    return oldFunc;
}

void PlatformUtils::shutdownWDT(PlatformUtils::WDT_EarlyWarning_Func replacementFunc)
{
    if (replacementFunc == NULL) {
        // no other handlers -- so full shutdown
        WDT->CTRL.bit.ENABLE = 0;
    }
    s_WDT_EarlyWarning_Func = replacementFunc;
}

void PlatformUtils::clearWDT()
{
    WDTsync();
    WDT->CLEAR.bit.CLEAR = WDT_CLEAR_CLEAR_KEY_Val;
    PlatformUtils::nonConstSingleton().setCountdownVal(WDT_MaxTime_ms/EARLY_WARNING_TIME_MS);
}


void WDT_Handler (void)
{
    // this is the early warning ISR (the real WDT doesn't actually call anything
    // I can use -- it just issues the system reset)
  
    // Also note that this is the "hard-coded" handler -- it'll count multiple calls
    // to itself, up to WDT_MaxTime_ms (approx) then inform the application by calling the
    // app-registerd callback 

    long c = PlatformUtils::singleton().getCountdownVal();
    PlatformUtils::nonConstSingleton().setCountdownVal(--c);
    if ((c == 0) && (s_WDT_EarlyWarning_Func != NULL)) {
        s_WDT_EarlyWarning_Func();  // call apps' early warning callback

	// Note, in this case, I'm not going to reset the WDT counter -- the app can do it
	// if it wants, otherwise I want the system reset to proceed on the next event
    } else {
        // reset the WDT counter -- doing so while WDT is busy will *stall* the CPU and interrupts
        // leading to very odd (and difficult to track down!) results.
        WDTsync();
        WDT->CLEAR.bit.CLEAR = WDT_CLEAR_CLEAR_KEY_Val;
    }

    // clear the interrupt so future ones can happen -- this happens regardless of whether
    // the WDT has been reset or not
    WDT->INTFLAG.bit.EW = 1;
}


