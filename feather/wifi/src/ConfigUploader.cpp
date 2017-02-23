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

#include <http_couchget.h>
#include <http_couchput.h>

#include <strutils.h>

#include <MyWiFi.h>



ConfigUploader::ConfigUploader(const HiveConfig &config,
			       const class RateProvider &rateProvider,
			       const class TimeProvider &timeProvider,
			       unsigned long now)
  : Sensor("config-uploader", rateProvider, timeProvider, now),
    mConfig(config), mDoUpload(false), mNextActionTime(0),
    mGetter(0), mPutter(0)
{}


ConfigUploader::~ConfigUploader()
{
    assert(mGetter == NULL, "mGetter == NULL");
    assert(mPutter == NULL, "mPutter == NULL");
    delete mGetter;
    delete mPutter;
}


class ConfigGetter : public HttpCouchGet {
private:
    bool mHasConfig, mParsed;
    CouchUtils::Doc mConfigDoc;
  
public:
    ConfigGetter(const char *ssid, const char *pswd, const char *dbHost, int dbPort, bool isSSL,
		 const char *url, const char *dbUser, const char *dbPswd)
      : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, dbUser, dbPswd, isSSL),
	mHasConfig(false), mParsed(false)
    {
        TF("ConfigGetter::ConfigGetter");
	TRACE("entry");
    }

    const CouchUtils::Doc getConfig() const {return mConfigDoc;}

    bool isError() const {return m_consumer.isError();}
    bool hasNotFound() const {return m_consumer.hasNotFound();}
    bool isTimeout() const {return m_consumer.isTimeout();}

    const Str &getFullResponse() const {return m_consumer.getResponse();}
  
    bool hasConfig() const {
        TF("ConfigGetter::hasConfig");
	
	if (mParsed)
	    return mHasConfig;
	
	ConfigGetter *nonConstThis = (ConfigGetter*)this;
	if (m_consumer.hasOk()) {
	    //TRACE(m_consumer.getResponse().c_str());

	    nonConstThis->mParsed = true;
	    const char *jsonStr = strstr(m_consumer.getResponse().c_str(), "\"_id\"");
	    if (jsonStr != NULL) {
		// work back to the beginning of the doc... then parse as a couch json doc!
		while ((jsonStr > m_consumer.getResponse().c_str()) && (*jsonStr != '{'))
		    --jsonStr;

                CouchUtils::parseDoc(jsonStr, &nonConstThis->mConfigDoc);
		nonConstThis->mHasConfig = true;
		return true;
	    }
	}

	return false;
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
    //TRACE("creating getter");

    // curl -X GET 'http://jfcenterprises.cloudant.com/hive-config/<serial#>'
	  
    Str url, encodedUrl;
    url.append(config.getHiveId());
    CouchUtils::urlEncode(url.c_str(), &encodedUrl);
	
    url.clear();
    CouchUtils::toURL(config.getConfigDbName(), encodedUrl.c_str(), &url);
    TRACE2("URL: ", url.c_str());
	
    return new ConfigGetter(config.getSSID(), config.getPSWD(),
			    config.getDbHost(), config.getDbPort(), config.isSSL(),
			    url.c_str(), config.getDbUser(), config.getDbPswd());
}


HttpCouchPut *ConfigUploader::createPutter(const HiveConfig &config, unsigned long now, const char *id, const char *rev)
{
    TF("ConfigUploader::createPutter");
    TRACE("creating putter");
    
    // curl -v -H "Content-Type: application/json" -X PUT "https://afteptsecumbehisomorther:e4f286be1eef534f1cddd6240ed0133b968b1c9a@jfcenterprises.cloudant.com:443/hive-config" -d '{"hiveid":"F0-17-66-FC-5E-A1","sensor":"heartbeat","timestamp":"1485894682","value":"0.9.104"}'
  
    Str url("/");
    url.append(config.getConfigDbName());
    url.append("/");
    url.append(id);

    TRACE2("url: ", url.c_str());

    CouchUtils::Doc uploadDoc;
    for (int i = 0; i < config.getDoc().getSz(); i++) {
        const CouchUtils::NameValuePair &p = config.getDoc()[i];
	if ((strcmp(p.getName().c_str(), "_id") != 0) &&
	    (strcmp(p.getName().c_str(), "_rev") != 0) &&
	    (strcmp(p.getName().c_str(), "timestamp") != 0)) {
	    // copy the nvpair
	    uploadDoc.addNameValue(new CouchUtils::NameValuePair(p));
	}
    }
    if (rev != NULL)
        uploadDoc.addNameValue(new CouchUtils::NameValuePair("_rev", CouchUtils::Item(rev)));

    Str timestr;
    if (getTimeProvider().haveTimestamp()) {
        getTimeProvider().toString(now, &timestr);
	uploadDoc.addNameValue(new CouchUtils::NameValuePair("timestamp", CouchUtils::Item(timestr)));
    }
	    
    Str dump;
    CouchUtils::toString(uploadDoc, &dump);
    TRACE2("doc to upload: ", dump.c_str());
	    
    return new HttpCouchPut(config.getSSID(), config.getPSWD(),
			    config.getDbHost(), config.getDbPort(),
			    url.c_str(), uploadDoc, 
			    config.getDbUser(), config.getDbPswd(), 
			    config.isSSL());
}


bool ConfigUploader::processGetter(const HiveConfig &config, unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("ConfigUploader::processGetter");
    
    HttpCouchGet::EventResult er = mGetter->event(now, callMeBackIn_ms);
    //TRACE2("event result: ", er);
    
    if (mGetter->processEventResult(er))
        return true; // not done yet
    
    TRACE("getter is done"); // somehow, we're done for now
    bool retry = true; // more true cases than false
    if (mGetter->hasConfig()) {
        const CouchUtils::Doc &configDoc = mGetter->getConfig();
	int idIndex = configDoc.lookup("_id");
	int revIndex = configDoc.lookup("_rev");
	if ((idIndex >= 0) && configDoc[idIndex].getValue().isStr() &&
	    configDoc[idIndex].getValue().getStr().equals(config.getHiveId()) &&
	    (revIndex >= 0) && configDoc[revIndex].getValue().isStr()) {
	    const Str &rev = configDoc[revIndex].getValue().getStr();
	    mPutter = createPutter(config, now, config.getHiveId(), rev.c_str());
	    retry = false;
	} else {
	    Str dump;
	    PH2("configDoc parsing error: ", CouchUtils::toString(configDoc, &dump));
	}
    } else {
        if (mGetter->isTimeout()) {
	    TRACE("Cannot access db");
	} else if (mGetter->hasNotFound()) {
	    TRACE("Doc doesn't exist in db");
	    mPutter = createPutter(config, now, config.getHiveId(), NULL);
	    retry = false;
	} else if (mGetter->isError()) {
	    PH2("doc not found in the response: ", mGetter->getFullResponse().c_str());
	}
    }
    if (retry) {
        TRACE("retrying again in 5s");
	*callMeBackIn_ms = 5000l;
    } else {
        assert(mPutter, "mPutter");
	*callMeBackIn_ms = 10l;
    }
    delete mGetter;
    mGetter = NULL;

    return false;
}


bool ConfigUploader::processPutter(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("ConfigUploader::processPutter");

    HttpCouchPut::EventResult er = mPutter->event(now, callMeBackIn_ms);
    //TRACE2("event result: ", er);
    if (mPutter->processEventResult(er))
        return true; // not done yet

    TRACE("putter is done");
    TRACE2("putter response: ", mPutter->getHeaderConsumer().getResponse().c_str());

    if (mPutter->getHeaderConsumer().hasOk()) {
        TRACE("hasOk; upload succeeded");
	mDoUpload = false;
    } else {
        TRACE("PUT failed; retrying again in 5s");
	*callMeBackIn_ms = 5000l;
    }
    delete mPutter;
    mPutter = NULL;
    return false;
}


bool ConfigUploader::loop(unsigned long now, Mutex *wifiMutex)
{
    TF("ConfigUploader::loop");

    bool callMeBack = true;
    if (mDoUpload && (now > mNextActionTime) && wifiMutex->own(this)) {
        //TRACE("doing upload");
	unsigned long callMeBackIn_ms = 10l;
	if (mPutter == NULL) {
	    //TRACE("no putter yet");
	    if (mGetter == NULL) {
	        //TRACE("creating getter");
	        mGetter = createGetter(mConfig);
	    } else {
	        //TRACE("processing getter");
	        callMeBack = processGetter(mConfig, now, &callMeBackIn_ms);
	    }
	} else {
	    //TRACE("processing putter");
	    callMeBack = processPutter(now, &callMeBackIn_ms);
	}
	mNextActionTime = now + callMeBackIn_ms;
    }
    if (!callMeBack)
        wifiMutex->release(this);
    return callMeBack;
}
