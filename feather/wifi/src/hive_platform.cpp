#include <hive_platform.h>

#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include <pulsegen_consumer.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <SdFat.h>

#include <platformutils.h>

#include <sdutils.h>


#define STACKTRACE_FILENAME "/STACK.LOG"

static HivePlatform *sPlatform = 0;
static PulseGenConsumer **sConsumers11K = 0;
static PulseGenConsumer **sConsumers22K = 0;
static PulseGenConsumer **sConsumers11K_stage = 0;
static PulseGenConsumer **sConsumers22K_stage = 0;


/* STATIC */
const HivePlatform *HivePlatform::singleton()
{
    if (sPlatform == 0)
        sPlatform = new HivePlatform();
    
    return sPlatform;
}

/* STATIC */
HivePlatform *HivePlatform::nonConstSingleton()
{
    if (sPlatform == 0)
        sPlatform = new HivePlatform();
    
    return sPlatform;
}


HivePlatform::HivePlatform()
{
    TF("HivePlatform::HivePlatform");
    TRACE("entry");
    
    sPlatform = this;

    sConsumers11K = new PulseGenConsumer*[10];
    sConsumers22K = new PulseGenConsumer*[10];
    sConsumers11K_stage = new PulseGenConsumer*[10];
    sConsumers22K_stage = new PulseGenConsumer*[10];
    for (int i = 0; i < 10; i++) {
      sConsumers11K[i] = sConsumers22K[i] = sConsumers11K_stage[i] = sConsumers22K_stage[i] = 0;
    }
}


const char *HivePlatform::getResetCause() const
{
    return PlatformUtils::singleton().resetCause(); // capture the reason for reset so I can inform the app
}


static void stackDump()
{
    if (TraceScope::sCurrScope != NULL) {
        SdFile f;
	bool canWrite = true;
	if (!f.open(STACKTRACE_FILENAME, O_CREAT | O_WRITE)) {
	    canWrite = false;
	}
	
        P("INFO: "); PL(millis());
        PL("FAULT: Stack trace: ");
	const TraceScope *s = TraceScope::sCurrScope;
	while (s != NULL) {
	    PL(s->getFunc());
	    if (canWrite) {
	        f.write(s->getFunc(), strlen(s->getFunc()));
		f.write("\n", 2);
	    }
	    s = s->getParent();
	}
	if (canWrite) {
	    f.close();
	}
    }
}


void HEADLESS_wdtEarlyWarningHandler()
{
    // first, prevent the WDT from doing a full system reset by resetting the timer
    PlatformUtils::nonConstSingleton().clearWDT();

    stackDump();
    
    // force an immediate system reset 
    NVIC_SystemReset();
}

void DEBUG_wdtEarlyWarningHandler()
{
    // first, prevent the WDT from doing a full system reset by resetting the timer
    PlatformUtils::nonConstSingleton().clearWDT();

    stackDump();
    
    PL("wdtEarlyWarningHandler; BARK!");
    PL("");

    if (PlatformUtils::s_traceStr != NULL) {
        P("WDT Trace message: ");
	PL(PlatformUtils::s_traceStr);
	P("WDT last mark time: ");
	PL(PlatformUtils::s_traceTime);
	P("now: ");
	PL(millis());
    } else {
        PL("No WDT trace message registered");
    }
    
    // Next, do a more useful system reset
    PlatformUtils::nonConstSingleton().resetToBootloader();
}


void HivePlatform::startWDT()
{
    DL("starting WDT");

#ifdef HEADLESS    
    PlatformUtils::nonConstSingleton().initWDT(HEADLESS_wdtEarlyWarningHandler);
#else
    PlatformUtils::nonConstSingleton().initWDT(DEBUG_wdtEarlyWarningHandler);
#endif
    
    WDT_TRACE("wdt handler registered");
}


void HivePlatform::clearWDT() 
{
    PlatformUtils::nonConstSingleton().clearWDT();
}


void HivePlatform::markWDT(const char *msg) const
{
    PlatformUtils::singleton().markWDT(msg, millis());
}


void HivePlatform::trace(const char *msg) 
{
    WDT_TRACE(msg);
}


void HivePlatform::error(const char *err) 
{
    WDT_TRACE(err);
    PL(err);
    while (1);
}



void HivePlatform::registerPulseGenConsumer_11K(class PulseGenConsumer *consumer)
{
    TF("HivePlatform::registerPulseGenConsumer_11K");
    TRACE("registering a consumer");
    
    int i = 0;
    while ((sConsumers11K[i] != 0) && (sConsumers11K[i] != consumer)) {
        i++;
    }
    if (sConsumers11K[i] == 0) {
        D("Registered pulseGenConsumer 11K consumer "); DL(i);
        sConsumers11K[i] = consumer;
    }
}


void HivePlatform::registerPulseGenConsumer_22K(class PulseGenConsumer *consumer)
{
    TF("HivePlatform::registerPulseGenConsumer_22K");
    TRACE("registering a consumer");
    
    int i = 0;
    while ((sConsumers22K[i] != 0) && (sConsumers22K[i] != consumer)) {
        i++;
    }
    if (sConsumers22K[i] == 0) {
        D("Registered pulseGenConsumer 22K consumer "); DL(i);
        sConsumers22K[i] = consumer;
    }
}


void HivePlatform::unregisterPulseGenConsumer_11K(class PulseGenConsumer *consumer)
{
    TF("HivePlatform::registerPulseGenConsumer_11K");
    TRACE("unregistering a consumer");

    int i = 0, j = 0;
    while (sConsumers11K[i]) {
        if (sConsumers11K[i] == consumer) i++;
	else sConsumers11K_stage[j++] = sConsumers11K[i++];
    }
    sConsumers11K_stage[j] = 0;

    PulseGenConsumer **t = sConsumers11K;
    sConsumers11K = sConsumers11K_stage;
    sConsumers11K_stage = t;
}


void HivePlatform::unregisterPulseGenConsumer_22K(class PulseGenConsumer *consumer)
{
    TF("HivePlatform::registerPulseGenConsumer_22K");
    TRACE("unregistering a consumer");

    int i = 0, j = 0;
    while (sConsumers22K[i]) {
        if (sConsumers22K[i] == consumer) i++;
	else sConsumers22K_stage[j++] = sConsumers22K[i++];
    }
    sConsumers22K_stage[j] = 0;

    PulseGenConsumer **t = sConsumers22K;
    sConsumers22K = sConsumers22K_stage;
    sConsumers22K_stage = t;
}


static void notifyConsumers()
{
    static bool sIsOdd = false;

    unsigned long now = millis();
    int i;

    if (sIsOdd) {
        i = 0;
	while (sConsumers11K[i] != 0) 
	    sConsumers11K[i++]->pulse(now);
    }
    sIsOdd = !sIsOdd;

    i = 0;
    while (sConsumers22K[i] != 0) 
        sConsumers22K[i++]->pulse(now);
}


void HivePlatform::pulseGen_22K_init()
{
    TF("HivePlatform::pulseGen_22K_init");
    WDT_TRACE("HivePlatform::pulseGen_22K_init(); initializing pulse generator");
    PlatformUtils::nonConstSingleton().initPulseGenerator(0, SAMPLES_PER_SECOND_22K);
    PlatformUtils::nonConstSingleton().startPulseGenerator(0, notifyConsumers);
    WDT_TRACE("HivePlatform::pulseGen_22K_init(); initialized pulse generator");
}
