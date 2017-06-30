#include <ConfigUploader.h>


#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hiveconfig.h>
#include <docwriter.h>
#include <couchutils.h>
#include <Mutex.h>

#include <TimeProvider.h>

#include <http_jsonget.h>
#include <http_couchput.h>

#include <strbuf.h>
#include <strutils.h>


ConfigUploader::ConfigUploader(const HiveConfig &config,
			       const class RateProvider &rateProvider,
			       unsigned long now, Mutex *wifi)
  : Sensor("config-uploader", rateProvider, now),
    mConfig(config), mDoUpload(false), mNextActionTime(now + 10000l),
    mGetter(0), mPutter(0), mWifiMutex(wifi), mDocToUpload(0)
{}


ConfigUploader::~ConfigUploader()
{
    assert(mGetter == NULL, "mGetter == NULL");
    assert(mPutter == NULL, "mPutter == NULL");
    delete mGetter;
    delete mPutter;
}


class ConfigGetter : public HttpJSONGet {
private:
    bool mHasConfig, mParsed;
    CouchUtils::Doc mConfigDoc;
  
public:
    ConfigGetter(const Str &ssid, const Str &pswd, const Str &dbHost, int dbPort, bool isSSL,
		 const Str &dbUser, const Str &dbPswd, const char *urlPieces[])
      : HttpJSONGet(ssid, pswd, dbHost, dbPort, dbUser, dbPswd, isSSL, urlPieces),
	mHasConfig(false), mParsed(false)
    {
        TF("ConfigGetter::ConfigGetter");
    }

    const CouchUtils::Doc &getConfig() const {return getJSONConsumer().getDoc();}

    bool isError() const {return getJSONConsumer().isError();}
    bool hasNotFound() const {return getJSONConsumer().hasNotFound();}
    bool isTimeout() const {return getJSONConsumer().isTimeout();}

    bool hasConfig() const {
        return getJSONConsumer().hasOk() && getJSONConsumer().hasDoc();
    }
};



bool ConfigUploader::isItTimeYet(unsigned long now)
{
    return mDoUpload;
}


void ConfigUploader::upload()
{
    TF("ConfigUploader::upload");
    PH("INFO: Instructed to upload the config at the next opportunity");
    mDoUpload = true;
    mNextActionTime = millis();
}


ConfigGetter *ConfigUploader::createGetter(const HiveConfig &config)
{
    TF("ConfigUploader::createGetter");
    
    // curl -X GET 'http://jfcenterprises.cloudant.com/hive-config/<serial#>'
	  
    static const char *urlPieces[5];
    urlPieces[0] = "/";
    urlPieces[1] = config.getConfigDbName().c_str();
    urlPieces[2] = urlPieces[0];
    urlPieces[3] = config.getHiveId().c_str();
    urlPieces[4] = 0;
    
    ConfigGetter *g = new ConfigGetter(config.getSSID(), config.getPSWD(),
				       config.getDbHost(), config.getDbPort(), config.isSSL(),
				       config.getDbUser(), config.getDbPswd(), urlPieces);
    return g;
}


HttpCouchPut *ConfigUploader::createPutter(const HiveConfig &config, CouchUtils::Doc *docToUpload, const char *id)
{
    TF("ConfigUploader::createPutter");
    TRACE("creating putter");

    static const char *urlPieces[5];
    urlPieces[0] = "/";
    urlPieces[1] = config.getConfigDbName().c_str();
    urlPieces[2] = urlPieces[0];
    urlPieces[3] = id;
    urlPieces[4] = 0;
    
#ifndef HEADLESS    
    CouchUtils::println(*docToUpload, Serial, "ConfigUploader::createPutter; doc to upload: ");
#endif
      
    return new HttpCouchPut(config.getSSID(), config.getPSWD(),
			    config.getDbHost(), config.getDbPort(),
			    docToUpload, 
			    config.getDbUser(), config.getDbPswd(), 
			    config.isSSL(), urlPieces);
}


void ConfigUploader::prepareDocToUpload(const CouchUtils::Doc &existingDoc,
					CouchUtils::Doc *newDoc,
					unsigned long now, const char *rev)
{
    TF("ConfigUploader::prepareDocToUpload");
    
    for (int i = 0; i < existingDoc.getSz(); i++) {
        const CouchUtils::NameValuePair &p = existingDoc[i];
	const Str &name = p.getName();
	if (!name.equals("_id") && !name.equals("_rev") && !name.equals(HiveConfig::TimestampProperty)) {
	    // copy the nvpair
	    newDoc->addNameValue(new CouchUtils::NameValuePair(p));
	}
    }
    if (rev != NULL)
        newDoc->addNameValue(new CouchUtils::NameValuePair("_rev", CouchUtils::Item(rev)));

    Str timestr;
    if (GetTimeProvider()) {
        GetTimeProvider()->toString(now, &timestr);
	newDoc->addNameValue(new CouchUtils::NameValuePair(HiveConfig::TimestampProperty, CouchUtils::Item(timestr)));
    }
}


bool ConfigUploader::processGetter(const HiveConfig &config, unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("ConfigUploader::processGetter");

    HttpCouchGet::EventResult er = mGetter->event(now, callMeBackIn_ms);
    
    if (mGetter->processEventResult(er))
        return true; // not done yet

    
    TRACE("getter is done"); // somehow, we're done getting -- see if and what more we can do
    bool retry = true; // more true cases than false
    if (mGetter->hasConfig()) {
        const CouchUtils::Doc &configDoc = mGetter->getConfig(); // use getter's result just to get the revision
	int revIndex = configDoc.lookup("_rev");
	if ((revIndex >= 0) && configDoc[revIndex].getValue().isStr()) {
	    const Str &rev = configDoc[revIndex].getValue().getStr();
	    mDocToUpload = new CouchUtils::Doc();
	    prepareDocToUpload(config.getDoc(), mDocToUpload, now, rev.c_str());
	    retry = false;
	} else {
	    StrBuf dump;
	    PH2("configDoc parsing error: ", CouchUtils::toString(configDoc, &dump));
	}
    } else {
        if (mGetter->isTimeout()) {
	    PH("Cannot access db");
	} else if (mGetter->hasNotFound()) {
	    TRACE("Doc doesn't exist in db");
	    mDocToUpload = new CouchUtils::Doc();
	    prepareDocToUpload(mGetter->getConfig(), mDocToUpload, now, NULL);
	    retry = false;
	} else if (mGetter->isError()) {
	    PH2("doc not found in the response; header: ", mGetter->getHeaderConsumer().getHeader().c_str());
	}
    }
    if (retry) {
        PH("retrying again in 5s");
	*callMeBackIn_ms = 5000l;
    } else {
	*callMeBackIn_ms = 10l;
    }

    mGetter->shutdownWifiOnDestruction(retry);
    delete mGetter;
    mGetter = NULL;

    return false;
}


bool ConfigUploader::processPutter(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("ConfigUploader::processPutter");

    HttpCouchPut::EventResult er = mPutter->event(now, callMeBackIn_ms);
    if (mPutter->processEventResult(er))
        return true; // not done yet

    TRACE2("putter response header: ", mPutter->getHeaderConsumer().getHeader().c_str());
    TRACE2("putter response content: ", mPutter->getCouchConsumer().getContent().c_str());

    bool retry = true;
    *callMeBackIn_ms = 5000l;
    if (mPutter->getHeaderConsumer().hasOk()) {
        TRACE("hasOk; upload succeeded");
	mDoUpload = false;
	retry = false;
	*callMeBackIn_ms = 10l;
    } else if (mPutter->isTimeout()) {
        PH("timeout; upload failed");
    } else if (mPutter->isError()) {
        PH("error; upload failed");
    } else {
        PH("PUT failed for unknown reasons; retrying again in 5s");
    }

    mPutter->shutdownWifiOnDestruction(retry);
    delete mPutter;
    mPutter = NULL;
    return false;
}


bool ConfigUploader::loop(unsigned long now)
{
    TF("ConfigUploader::loop");
    bool callMeBack = true;
    if (mDoUpload && (now > mNextActionTime) && mWifiMutex->own(this)) {
        //TRACE("doing upload");
	unsigned long callMeBackIn_ms = 10l;
	if (mPutter == NULL) {
	    //TRACE("no putter yet");
	    if ((mGetter == NULL) && (mDocToUpload == NULL)) {
	        //TRACE("creating getter");
	        mGetter = createGetter(mConfig);
	    } else {
	        if (mDocToUpload == NULL) {
		    //TRACE("processing getter");
		    callMeBack = processGetter(mConfig, now, &callMeBackIn_ms);
		} else {
		    mPutter = createPutter(mConfig, mDocToUpload, mConfig.getHiveId().c_str());
		    mDocToUpload = NULL; // control passed to HttpCouchPut
		    callMeBack = true;
		}
	    }
	} else {
	    //TRACE("processing putter");
	    callMeBack = processPutter(now, &callMeBackIn_ms);
	}
	mNextActionTime = now + callMeBackIn_ms;
    }
    if (!callMeBack)
        mWifiMutex->release(this);

    return callMeBack;
}
