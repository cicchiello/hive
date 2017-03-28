#include <ConfigPersister.h>


#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hiveconfig.h>
#include <docwriter.h>
#include <couchutils.h>
#include <Mutex.h>

#include <http_couchget.h>
#include <http_couchput.h>

#include <strbuf.h>
#include <strutils.h>



ConfigPersister::ConfigPersister(const HiveConfig &config,
				 const class RateProvider &rateProvider,
				 const char *configFilename, 
				 unsigned long now, Mutex *sdMutex)
  : Sensor("config-persister", rateProvider, now),
    mConfig(config), mDoPersist(false), 
    mSdMutex(sdMutex), mWriter(0), mConfigFilename(configFilename)
{}


ConfigPersister::~ConfigPersister()
{
    delete mWriter;
}


bool ConfigPersister::isItTimeYet(unsigned long now)
{
    return mDoPersist;
}


void ConfigPersister::persist()
{
    TF("ConfigPersister::persist");
    PH("INFO: Instructed to persist the config at the next opportunity");
    mDoPersist = true;
}


bool ConfigPersister::loop(unsigned long now)
{
    TF("ConfigPersister::loop");

    bool callMeBack = true;
    if (mDoPersist && mSdMutex->isAvailable()) {
        TRACE("doing persist");
	unsigned long callMeBackIn_ms = 10l;
	if (mWriter == NULL) {
	    TRACE("creating DocWriter");
	    mWriter = new DocWriter(mConfigFilename, mConfig.getDoc(), mSdMutex);
	} else {
	    callMeBack = mWriter->loop();
	    if (!callMeBack) {
	        TRACE("Done persisting config");
		mDoPersist = false;
		delete mWriter;
		mWriter = NULL;
	    }
	}
    }

    return callMeBack;
}
