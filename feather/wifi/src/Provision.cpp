#include <Provision.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hiveconfig.h>
#include <ConfigReader.h>
#include <ConfigWriter.h>
#include <couchutils.h>
#include <Mutex.h>

#include <http_couchget.h>
#include <http_couchput.h>

#define LOAD_CONFIG      1
#define WRITE_CONFIG     2
#define DOWNLOAD_CONFIG  3
#define UPLOAD_CONFIG    4
#define DONE             5

#define INIT             1
#define INIT_DEFAULT     2
#define LOADING          3
#define WRITING          4


#ifndef NULL
#define NULL 0
#endif

static Provision *s_singleton = NULL;

Provision::Provision(const char *resetCause, const char *versionId, const char *configFilename, unsigned long now)
  : Actuator("provisioner", now), mNextActionTime(0), mConfigReader(0),
    mConfig(new HiveConfig(resetCause, versionId)),
    mResetCause(resetCause), mVersionId(versionId), mConfigFilename(configFilename), mRetryCnt(0),
    mGetter(0), mPutter(0)
{
    TF("Provision::Provision");
    
    if (s_singleton != NULL) {
        ERR("Provision class violated implied singleton rule");
    }
}


Provision::~Provision()
{
    delete mConfigReader;
    delete mConfigWriter;
    delete mConfig;
    s_singleton = NULL;
}


void Provision::startConfiguration()
{
    mMajorState = LOAD_CONFIG;
    mMinorState = INIT;
}


bool Provision::isItTimeYet(unsigned long now) const
{
    return now >= mNextActionTime;
}


bool Provision::loop(unsigned long now, Mutex *wifi)
{
    switch (mMajorState) {
    case LOAD_CONFIG: return loadLoop(now);
    case DOWNLOAD_CONFIG: return downloadLoop(now, wifi);
    case WRITE_CONFIG: return writeLoop(now);
    case UPLOAD_CONFIG: return uploadLoop(now, wifi);
    case DONE: return false;
    }
}


bool Provision::loadLoop(unsigned long now)
{
    TF("Provision::loadLoop");
    TRACE("entry");
    
    bool shouldReturn = true; // very few cases shouldn't return, so make true the default
    switch (mMinorState) {
    case INIT: {
        mConfigReader = new ConfigReader(mConfigFilename.c_str());
	mConfigReader->setup();
	mMinorState = LOADING;
    }
	break;
	
    case LOADING: {
        bool callConfigReaderAgain = mConfigReader->loop();
	if (!callConfigReaderAgain) {
	    bool haveValidConfig = false;
	    if (mConfigReader->hasConfig()) {
	        mConfig->setDoc(mConfigReader->getConfig());

		Str dump;
		CouchUtils::toString(mConfig->getDoc(), &dump);
	        TRACE2("Have a local configuration: ", dump.c_str());
		
		haveValidConfig = mConfig->isValid();
		if (!haveValidConfig) {
		    TRACE("Invalid local config loaded");
		}
	    } else {
	        TRACE2("Error: ", mConfigReader->errMsg());
	    }
	    delete mConfigReader;
	    mConfigReader = NULL;
	    if (haveValidConfig) {
	        mMajorState = DOWNLOAD_CONFIG;
		mMinorState = INIT;
	    } else {
	        mMajorState = WRITE_CONFIG;
		mPostWriteState = DOWNLOAD_CONFIG;
		mMinorState = INIT_DEFAULT;
	    }
	} else {
	    TRACE("configReader wants me to call it back");
	}
    }
	break;

    default:
        TRACE("Shouldn't get here");
    }

    mNextActionTime = now + 10l;
    return shouldReturn;
}


bool Provision::writeLoop(unsigned long now)
{
    TF("Provision::writeLoop");
    TRACE("entry");
    
    bool shouldReturn = true; // very few cases shouldn't return, so make true the default
    switch (mMinorState) {
    case INIT_DEFAULT: {
        TRACE("Creating a default config file, then retrying the load");
	HiveConfig defaultConfig(mResetCause.c_str(), mVersionId.c_str());
	mConfigWriter = new ConfigWriter(mConfigFilename.c_str(), defaultConfig.getDoc());
	mMinorState = WRITING;
    }
	break;
	
    case INIT: {
        TRACE("Writing the current HiveConfig to file");
	mConfigWriter = new ConfigWriter(mConfigFilename.c_str(), mConfig->getDoc());
	mMinorState = WRITING;
    }
	break;
	
    case WRITING: {
        bool callWriterAgain = mConfigWriter->loop();
	if (!callWriterAgain) {
	    bool haveValidConfig = false;
	    if (!mConfigWriter->hasError()) {
	        TRACE("wrote local configuration");
		mConfig->setDoc(mConfigWriter->getConfig());
		delete mConfigWriter;
		mConfigWriter = NULL;

		haveValidConfig = mConfig->isValid();
	    } else {
	        TRACE2("Error: ", mConfigWriter->errMsg());
	    }
	    if (!haveValidConfig) {
	        TRACE("Using default local configuration");
		mConfig->setDefault();
	    }
	    mMajorState = mPostWriteState;
	    mMinorState = INIT;
	    TRACE("have a configuration");
	    mConfig->print();
	} else {
	    TRACE("configReader wants me to call it back");
	}
    }
	break;

    default:
        TRACE("Shouldn't get here");
    }

    mNextActionTime = now + 10l;
    return shouldReturn;
}



class ConfigGetter : public HttpCouchGet {
private:
    bool mHasConfig, mParsed;
    CouchUtils::Doc mConfigDoc;
  
public:
    ConfigGetter(const char *ssid, const char *pswd, const char *dbHost, int dbPort, bool isSSL,
		 const char *url, const char *credentials)
      : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, credentials, isSSL),
	mHasConfig(false), mParsed(false)
    {
        TF("ConfigGetter::ConfigGetter");
	TRACE("entry");
    }

    const CouchUtils::Doc getConfig() const {return mConfigDoc;}

    bool isError() const {return m_consumer.isError();}
    bool hasNotFound() const {return m_consumer.hasNotFound();}
    bool isTimeout() const {return m_consumer.isTimeout();}
  
    bool hasConfig() const {
        TF("ConfigGetter::hasConfig");
	TRACE("entry");
	
	if (mParsed)
	    return mHasConfig;
	
	ConfigGetter *nonConstThis = (ConfigGetter*)this;
	if (m_consumer.hasOk()) {
	    TRACE(m_consumer.getResponse().c_str());

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


bool Provision::downloadLoop(unsigned long now, Mutex *wifi)
{
    TF("Provision::downloadLoop");
    
    mNextActionTime = now + 10l;
    if (wifi->own(this)) {
        if (mGetter == NULL) {
	    //TRACE("creating getter");

	    // curl -X GET 'http://jfcenterprises.cloudant.com/hive-config/<serial#>'
	  
	    Str url, encodedUrl;
	    url.append(mConfig->getHiveId());
	    CouchUtils::urlEncode(url.c_str(), &encodedUrl);
	
	    url.clear();
	    CouchUtils::toURL(mConfig->getConfigDbName(), encodedUrl.c_str(), &url);
	    TRACE2("URL: ", url.c_str());
	
	    mGetter = new ConfigGetter(mConfig->getSSID(), mConfig->getPSWD(),
				       mConfig->getDbHost(), mConfig->getDbPort(), mConfig->isSSL(),
				       url.c_str(), mConfig->getDbCredentials());
	} else {
	    //TRACE("processing event");
	    unsigned long callMeBackIn_ms = 0;
	    HttpCouchGet::EventResult er = mGetter->event(now, &callMeBackIn_ms);
	    //TRACE2("event result: ", er);
	    if (!mGetter->processEventResult(er)) {
	        //TRACE("done");
		if (mGetter->hasConfig()) {
		    if (mConfig->getDoc().equals(mGetter->getConfig())) {
		        TRACE("downloaded config exactly matches filed config; done Provisioning...");
			mMajorState = DONE;
			mMinorState = INIT;
		    } else {
		        TRACE("downloaded config differs from filed config; saving it to file...");
			mConfig->setDoc(mGetter->getConfig());

			mMajorState = WRITE_CONFIG;
			mPostWriteState = DONE;
			mMinorState = INIT;
		    }
		} else {
		    bool retry = false;
		    if (mGetter->isTimeout()) {
		        TRACE("Cannot access db");
			retry = true;
		    } else if (mGetter->hasNotFound()) {
		        TRACE("Doc doesn't exist in db");
		    } else if (mGetter->isError()) {
		        TRACE("Doc not found in the response");
			retry = (++mRetryCnt < 3);
			if (!retry) {
			    TRACE("Retry limit exceeded");
			}
		    }
		    if (retry) {
		        TRACE("retrying again in 5s");
			callMeBackIn_ms = 5000l;
		    } else {
			TRACE("Will use the local config and attempt to upload it to db");
			mMajorState = UPLOAD_CONFIG;
			mMinorState = INIT;
			mRetryCnt = 0;
			callMeBackIn_ms = 10l;
		    }
		}
		delete mGetter;
		mGetter = NULL;
		wifi->release(this);
	    }
	    mNextActionTime = now + callMeBackIn_ms;
	}
    }
    
    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}



bool Provision::uploadLoop(unsigned long now, Mutex *wifi)
{
    TF("Provision::uploadLoop");
    TRACE("entry");

    if (wifi->own(this)) {
        TRACE("working on putting...");
	unsigned long callMeBackIn_ms = 10l;
	if (mPutter == NULL) {
	    TRACE("creating putter");

	    // curl -v -H "Content-Type: application/json" -X PUT "https://afteptsecumbehisomorther:e4f286be1eef534f1cddd6240ed0133b968b1c9a@jfcenterprises.cloudant.com:443/hive-config" -d '{"hiveid":"F0-17-66-FC-5E-A1","sensor":"heartbeat","timestamp":"1485894682","value":"0.9.104"}'
  
	    Str url("/");
	    url.append(mConfig->getConfigDbName());
	    url.append("/");
	    url.append(mConfig->getHiveId());

	    TRACE2("url: ", url.c_str());

	    CouchUtils::Doc uploadDoc;
	    for (int i = 0; i < mConfig->getDoc().getSz(); i++) {
	      const CouchUtils::NameValuePair &p = mConfig->getDoc()[i];
		if ((strcmp(p.getName().c_str(), "_id") != 0) &&
		    (strcmp(p.getName().c_str(), "_rev") != 0)) {
		    // copy the nvpair
		    uploadDoc.addNameValue(new CouchUtils::NameValuePair(p));
		}
	    }
	    
	    Str dump;
	    CouchUtils::toString(uploadDoc, &dump);
	    TRACE2("doc: ", dump.c_str());
	    
	    mPutter = new HttpCouchPut(mConfig->getSSID(), mConfig->getPSWD(),
				       mConfig->getDbHost(), mConfig->getDbPort(),
				       url.c_str(), uploadDoc, 
				       mConfig->getDbCredentials(),
				       mConfig->isSSL());
	} else {
	    TRACE("processing event");
	    HttpCouchPut::EventResult er = mPutter->event(now, &callMeBackIn_ms);
	    TRACE2("event result: ", er);
	    if (!mPutter->processEventResult(er)) {
		TRACE2("response: ", mPutter->getHeaderConsumer().getResponse().c_str());
		TRACE("end of response");
		bool success = mPutter->getHeaderConsumer().hasOk();
		if (success || (++mRetryCnt == 3)) {
		  TRACE((success ? "hasOk; upload succeeded" :
			 "Retry limit exceeded; will just use the local config"));
		    mMajorState = DONE;
		    mMinorState = INIT;
		    mRetryCnt = 0;
		    callMeBackIn_ms = 10l;
		} else {
		    TRACE("PUT failed; retrying again in 5s");
		    callMeBackIn_ms = 5000l;
		}
		delete mPutter;
		mPutter = NULL;
		wifi->release(this);
	    }
	}
	mNextActionTime = now + callMeBackIn_ms;
    }
    
    return true; // one of parent loop func or this loop func will always want to be called again
}
