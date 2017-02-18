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

#include <platformutils.h>

#include <sdutils.h>

#include <SdFat.h>


#define STACKTRACE_FILENAME "/STACK.LOG"

static HivePlatform *sPlatform = 0;
static PulseGenConsumer **sConsumers10K = 0;
static PulseGenConsumer **sConsumers20K = 0;


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

    sConsumers10K = new PulseGenConsumer*[10];
    sConsumers20K = new PulseGenConsumer*[10];
    for (int i = 0; i < 10; i++) {
        sConsumers10K[i] = sConsumers20K[i] = 0;
    }
}


const char *HivePlatform::getResetCause() const
{
    return PlatformUtils::singleton().resetCause(); // capture the reason for reset so I can inform the app
}


void stackDump()
{
    if (TraceScope::sCurrScope != NULL) {
        SdFile f;
	bool canWrite = true;
	if (!f.open(STACKTRACE_FILENAME, O_CREAT | O_WRITE)) {
	    canWrite = false;
	}
	
        PL("wdtEarlyWarningHandler; scope stack trace: ");
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



void HivePlatform::registerPulseGenConsumer_10K(class PulseGenConsumer *consumer)
{
    TF("HivePlatform::registerPulseGenConsumer_10K");
    TRACE("registering a consumer");
    
    int i = 0;
    while ((sConsumers10K[i] != 0) && (sConsumers10K[i] != consumer)) {
        i++;
    }
    if (sConsumers10K[i] == 0) {
        D("Registered pulseGenConsumer 10K consumer "); DL(i);
        sConsumers10K[i] = consumer;
    }
}


void HivePlatform::registerPulseGenConsumer_20K(class PulseGenConsumer *consumer)
{
    TF("HivePlatform::registerPulseGenConsumer_20K");
    TRACE("registering a consumer");
    
    int i = 0;
    while ((sConsumers20K[i] != 0) && (sConsumers20K[i] != consumer)) {
        i++;
    }
    if (sConsumers20K[i] == 0) {
        D("Registered pulseGenConsumer 20K consumer "); DL(i);
        sConsumers20K[i] = consumer;
    }
}


static void notifyConsumers()
{
    static bool sIsOdd = false;

    HivePlatform::trace("::notifyConsumers; entry");
    
    unsigned long now = millis();
    int i;

    if (sIsOdd) {
        i = 0;
	while (sConsumers10K[i] != 0) 
	    sConsumers10K[i++]->pulse(now);
    HivePlatform::trace("::notifyConsumers; trace 3");
    }
    HivePlatform::trace("::notifyConsumers; trace 4");
    sIsOdd = !sIsOdd;

    i = 0;
    while (sConsumers20K[i] != 0) 
        sConsumers20K[i++]->pulse(now);
    
    HivePlatform::trace("::notifyConsumers; exit");
}

void HivePlatform::pulseGen_20K_init()
{
    TF("HivePlatform::pulseGen_20K_init");
    WDT_TRACE("HivePlatform::pulseGen_20K_init(); initializing pulse generator");
    PlatformUtils::nonConstSingleton().initPulseGenerator(0, SAMPLES_PER_SECOND_20K);
    PlatformUtils::nonConstSingleton().startPulseGenerator(0, notifyConsumers);
    WDT_TRACE("HivePlatform::pulseGen_20K_init(); initialized pulse generator");
}
