#include <ChangesGetter.h>


#include <Arduino.h>

//#define HEADLESS
//#define NDEBUG

#include <Trace.h>

#include <hiveconfig.h>
#include <docwriter.h>
#include <couchutils.h>
#include <Mutex.h>

#include <TimeProvider.h>

#include <http_couchget.h>
#include <http_couchput.h>

#include <strbuf.h>
#include <strutils.h>


ChangesGetter::ChangesGetter(const HiveConfig &config, unsigned long now, Mutex *wifi)
  : mConfig(config), mNextActionTime(now + 1000l), mGetter(0), mWifiMutex(wifi),
    mLastSeq(new StrBuf("")), mGetNextSet(false)
{}


ChangesGetter::~ChangesGetter()
{
    assert(mGetter == NULL, "mGetter == NULL");
    delete mGetter;
    delete mLastSeq;
}


class ChangeGetter : public HttpCouchGet {
private:
    bool mHasResultset, mParsed;
    CouchUtils::Doc mResultset;
  
public:
    ChangeGetter(const Str &ssid, const Str &pswd, const Str &dbHost, int dbPort, bool isSSL,
		 const char *url, const Str &dbUser, const Str &dbPswd)
      : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, dbUser, dbPswd, isSSL), mParsed(false)
    {
        TF("ChangeGetter::ChangeGetter");
    }

    bool isError() const {return m_consumer.isError();}
    bool hasNotFound() const {return m_consumer.hasNotFound();}
    bool isTimeout() const {return m_consumer.isTimeout();}

    bool hasResultset() const {
        TF("ChangeGetter::hasConfig");
	
	if (mParsed)
	    return mHasResultset;
	
	ChangeGetter *nonConstThis = (ChangeGetter*)this;
	if (m_consumer.hasOk()) {
	    nonConstThis->mParsed = true;
	    const char *jsonStr = strstr(m_consumer.getResponse().c_str(), "\"results\"");
	    if (jsonStr != NULL) {
		// work back to the beginning of the doc... then parse as a couch json doc!
		while ((jsonStr > m_consumer.getResponse().c_str()) && (*jsonStr != '{'))
		    --jsonStr;

                CouchUtils::parseDoc(jsonStr, &nonConstThis->mResultset);
		nonConstThis->mHasResultset = true;
		return true;
	    }
	}

	return false;
    }
  
    const CouchUtils::Doc getResultset() const {return mResultset;}
  
    const StrBuf &getFullResponse() const {return m_consumer.getResponse();}
};



bool ChangesGetter::isItTimeYet(unsigned long now)
{
    return now > mNextActionTime;
}


#define CHANGESET_LIMIT "limit=3"

ChangeGetter *ChangesGetter::createGetter(const HiveConfig &config)
{
    TF("ChangesGetter::createGetter");
    
    // curl -X GET https://jfcenterprises.cloudant.com/hive-channel/_changes?limit=5
	  
    StrBuf url2;
    CouchUtils::toURL(config.getChannelDbName().c_str(), "_changes", &url2);
    if (mLastSeq->len()) {
        url2.append("?since=").append(mLastSeq->c_str()).append("&").append(CHANGESET_LIMIT);
    } else {
        url2.append("?").append(CHANGESET_LIMIT);
//        const char *prevSeq = "1546-g1AAAAHBeJzLYWBgYMlgTmFQSklKzi9KdUhJMjLRy83PzyvOyMxL1UvOyS9NScwr0ctLLckBqmVKZEiy____f1YGcxKQx5ULFGNPNra0SEsyJ8oQNLuM8dmV5AAkk-oR1lVDrDM0SkoxSyTKHDTrDPBZl8cCJBkagBTQxv1AKxPvgO0zNrEwNEs0JsogVPsMzQnbdwBiH8iLiSvB9qUaGCRZpiYRZVAWAKIHkvo";
//	url2.append("?since=").append(prevSeq).append("&limit=3");
    }
    TRACE2("URL: ", url2.c_str());

    mGetNextSet = false;
    return new ChangeGetter(config.getSSID(), config.getPSWD(),
			    config.getDbHost(), config.getDbPort(), config.isSSL(),
			    url2.c_str(), config.getDbUser(), config.getDbPswd());
}


bool ChangesGetter::processGetter(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("ChangesGetter::processGetter");

    HttpCouchGet::EventResult er = mGetter->event(now, callMeBackIn_ms);
    
    if (mGetter->processEventResult(er))
        return true; // not done yet

    
    TRACE("getter is done"); // somehow, we're done getting -- see if and what more we can do
    bool retry = true; // more true cases than false
    if (mGetter->hasResultset()) {
        const CouchUtils::Doc &changeDoc = mGetter->getResultset();
	// use getter's result just to get the last seq #

	int lastSeqIndex = changeDoc.lookup("last_seq");
	int pendingIndex = changeDoc.lookup("pending");
	if ((lastSeqIndex >= 0) && changeDoc[lastSeqIndex].getValue().isStr() &&
	    (pendingIndex >= 0) && changeDoc[pendingIndex].getValue().isStr() &&
	    StringUtils::isNumber(changeDoc[pendingIndex].getValue().getStr().c_str())) {
	    *mLastSeq = changeDoc[lastSeqIndex].getValue().getStr().c_str();
            const char *pendingCStr = changeDoc[pendingIndex].getValue().getStr().c_str();
            int pending = atoi(pendingCStr);
PH2("pending: ", pending);	    
	    retry = false;
	    mHasLastSeqId = pending == 0;
	    mGetNextSet = !mHasLastSeqId;
	} else {
	    StrBuf dump;
	    PH2("ChangeDoc parsing error: ", CouchUtils::toString(changeDoc, &dump));
	}
    } else if (mGetter->isTimeout()) {
PH2("http timeout; response: ", mGetter->getHeaderConsumer().getResponse().c_str())
  
assert(0, "Stopping for now...");
    } else if (mGetter->hasNotFound()) {
PH2("http not found; response: ", mGetter->getHeaderConsumer().getResponse().c_str())
  
assert(0, "Stopping for now...");
    } else if (mGetter->isError()) {
PH2("http error; response: ", mGetter->getHeaderConsumer().getResponse().c_str())
  
assert(0, "Stopping for now...");
    } else {
PH2("http response: ", mGetter->getHeaderConsumer().getResponse().c_str())
  
assert(0, "Stopping for now...");
        if (mGetter->isTimeout()) {
	    PH("Cannot access db");
	} else {
	    PH2("ChangeDoc not found in the response: ", mGetter->getFullResponse().c_str());
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


bool ChangesGetter::loop(unsigned long now)
{
    TF("ChangesGetter::loop");

    bool callMeBack = true;
    if (mWifiMutex->own(this)) {
	unsigned long callMeBackIn_ms = 10l;
	if (mGetter == NULL) {
	    //TRACE("creating getter");
	    mGetter = createGetter(mConfig);
	} else {
	    callMeBack = processGetter(now, &callMeBackIn_ms);
	    if (!callMeBack && mGetNextSet) {
	        callMeBack = true;
	    }
	}
	mNextActionTime = now + callMeBackIn_ms;
    }
    if (!callMeBack)
        mWifiMutex->release(this);

    return callMeBack;
}
