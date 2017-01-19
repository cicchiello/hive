#include <hive_platform.h>

#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BluefruitLE_SPI.h"

#include "BluefruitConfig.h"

#include <pulsegen_consumer.h>


#define HEADLESS
#define NDEBUG


#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) do {} while (0)
#define PL(args) do {} while (0)
#endif

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#include <platformutils.h>


static HivePlatform *sPlatform = 0;


/* STATIC */
const HivePlatform *HivePlatform::singleton()
{
    return sPlatform;
}

/* STATIC */
HivePlatform *HivePlatform::nonConstSingleton()
{
    return sPlatform;
}


HivePlatform::HivePlatform()
{
    sPlatform = this;
}


const char *HivePlatform::getResetCause() const
{
    return PlatformUtils::singleton().resetCause(); // capture the reason for reset so I can inform the app
}


void HEADLESS_wdtEarlyWarningHandler()
{
    // force an immediate system reset 
    NVIC_SystemReset();
}

void DEBUG_wdtEarlyWarningHandler()
{
    // first, prevent the WDT from doing a full system reset by resetting the timer
    PlatformUtils::nonConstSingleton().clearWDT();

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


void HivePlatform::trace(const char *msg) const
{
    WDT_TRACE(msg);
}
 
void HivePlatform::error(const char *err) const
{
    WDT_TRACE(err);
    PL(err);
    while (1);
}



static PulseGenConsumer **sConsumers10K = 0;
static PulseGenConsumer **sConsumers20K = 0;

void HivePlatform::registerPulseGenConsumer_10K(class PulseGenConsumer *consumer)
{
    if (sConsumers10K == 0) {
        sConsumers10K = new PulseGenConsumer*[10];
        sConsumers20K = new PulseGenConsumer*[10];
	for (int i = 0; i < 10; i++)
	  sConsumers10K[i] = sConsumers20K[i] = 0;
    }

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
    if (sConsumers20K == 0) {
        sConsumers10K = new PulseGenConsumer*[10];
        sConsumers20K = new PulseGenConsumer*[10];
	for (int i = 0; i < 10; i++)
	  sConsumers10K[i] = sConsumers20K[i] = 0;
    }

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

    unsigned long now = millis();
    int i;
    
    if (sIsOdd) {
        i = 0;
	while (sConsumers10K[i] != 0) 
	    sConsumers10K[i++]->pulse(now);
    }
    sIsOdd = !sIsOdd;

    i = 0;
    while (sConsumers20K[i] != 0) 
        sConsumers20K[i++]->pulse(now);
}

void HivePlatform::pulseGen_20K_init()
{
    WDT_TRACE("HivePlatform::pulseGen_20K_init(); initializing pulse generator");
    PlatformUtils::nonConstSingleton().initPulseGenerator(0, SAMPLES_PER_SECOND_20K);
    PlatformUtils::nonConstSingleton().startPulseGenerator(0, notifyConsumers);
}
