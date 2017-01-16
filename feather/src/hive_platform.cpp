#include <hive_platform.h>

#include <Arduino.h>

#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BluefruitLE_SPI.h"

#include "BluefruitConfig.h"

#include <pulsegen_consumer.h>


//#define HEADLESS
//#define NDEBUG


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


#ifndef NDEBUG
#define assert(c,msg) if (!(c)) {PL("ASSERT"); WDT_TRACE(msg); while(1);}
#else
#define assert(c,msg) do {} while(0);
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
    PL(err);
    WDT_TRACE(err);
    while (1);
}



static PulseGenConsumer **sConsumers = 0;

void HivePlatform::registerPulseGenConsumer_10K(class PulseGenConsumer *consumer)
{
    if (sConsumers == 0) {
        sConsumers = new PulseGenConsumer*[10];
	for (int i = 0; i < 10; i++)
	  sConsumers[i] = 0;
    }

    int i = 0;
    while ((sConsumers[i] != 0) && (sConsumers[i] != consumer)) {
        i++;
    }
    if (sConsumers[i] == 0) {
        D("Registered pulseGenConsumer "); DL(i);
        sConsumers[i] = consumer;
    }
}


static void notifyConsumers()
{
    int i = 0;
    while (sConsumers[i] != 0) {
        PulseGenConsumer *consumer = sConsumers[i++];
        consumer->pulse();
    }
}

void HivePlatform::pulseGen_10K_init()
{
    WDT_TRACE("HivePlatform::pulseGen_10K_init(); initializing pulse generator");
    PlatformUtils::nonConstSingleton().initPulseGenerator(0, SAMPLES_PER_SECOND_10K);
    PlatformUtils::nonConstSingleton().startPulseGenerator(0, notifyConsumers);
}
