#include <Timestamp.h>

#include <Arduino.h>


//#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <str.h>

#include <rtcconversions.h>
#include <http_couchget.h>


static const char *TimestampDb = "persistent-enum";
static const char *TimestampDocId = "ok-doc";
static const char *DateTag = "Date: ";


class RTCGetter : public HttpCouchGet {
public:
    RTCGetter(const char *ssid, const char *pswd, const char *dbHost, int dbPort, bool isSSL,
	      const char *url, const char *credentials)
      : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, credentials, isSSL)
    {
        TF("RTCGetter::RTCGetter");
	TRACE("entry");
    }
 
    void resetForRetry()
    {
        TF("RTCGetter::resetForRetry");
	TRACE("entry");
	HttpCouchGet::resetForRetry();
    }
  
    Str getTimestamp() const {
        TF("RTCGetter::getTimestamp");
	TRACE("entry");
        const char *dateStr = strstr(m_consumer.getResponse().c_str(), DateTag);
	if (dateStr != NULL) {
	    dateStr += strlen(DateTag);
	    Str date;
	    while (*dateStr != 13) date.add(*dateStr++);
	    TRACE2("Received timestamp: ", date.c_str());
	    return date;
	} else {
	    return Str("unknown");
	}
    }

    bool hasTimestamp() const {
        TF("RTCGetter::hasTimestamp");
	TRACE2("consumer response: ", m_consumer.getResponse().c_str());
        const char *dateStr = strstr(m_consumer.getResponse().c_str(), DateTag);
	return dateStr != NULL;
    }
};



Timestamp::Timestamp(const char *ssid, const char *pswd, const char *dbHost, const int dbPort, bool isSSL,
		     const char *dbCredentials)
  : mGetter(NULL),
    mSsid(new Str(ssid)), mPswd(new Str(pswd)), mDbHost(new Str(dbHost)), mDbPort(dbPort), mIsSSL(isSSL),
    mDbCredentials(new Str(dbCredentials)),
    mNextAttempt(0), mHaveTimestamp(false)
{
}

Timestamp::~Timestamp()
{
    delete mSsid;
    delete mPswd;
    delete mDbHost;
    delete mDbCredentials;
}

bool Timestamp::loop(unsigned long now)
{
    TF("Timestamp::loop");
    bool callMeBack = true;
    if (mNextAttempt == 0) {
        mNextAttempt = now + 1000l; // delay for 1s before trying to use the wifi
    } else if (now > mNextAttempt && !mHaveTimestamp) {
        if (mGetter == NULL) {
	    TRACE("creating getter");
	    Str url;
	    CouchUtils::toURL(TimestampDb, TimestampDocId, &url);
	    mGetter = new RTCGetter(mSsid->c_str(), mPswd->c_str(),
				    mDbHost->c_str(), mDbPort, mIsSSL,
				    url.c_str(), mDbCredentials->c_str());
	} else {
	    unsigned long callMeBackIn_ms = 0;
	    HttpOp::EventResult er = mGetter->event(now, &callMeBackIn_ms);
	    if (!mGetter->processEventResult(er)) {
	        TRACE("done");
		bool retry = false;
		if (mGetter->hasTimestamp()) {
		    Str timestampStr = mGetter->getTimestamp();
		    TRACE2("TimestampStr: ", timestampStr.c_str());
		    bool stat = RTCConversions::cvtToTimestamp(timestampStr.c_str(), &mTimestamp);
		    if (stat) {
		        TRACE2("Timestamp: ", mTimestamp);
			mSecondsAtMark = (millis()+500)/1000;
			mHaveTimestamp = true;
			callMeBack = false;
		    } else {
		        PL("Conversion to unix timestamp failed; retrying again in 5s");
			retry = true;
		    }
		} else {
		    PL("Timestamp lookup failed; retrying again in 5s");
		    retry = true;
		}
		if (retry) {
		    TRACE("setting up for a retry");
		    mTimestamp = 0;
		    callMeBackIn_ms = 5000l;
		}
		delete mGetter;
		mGetter = NULL;
	    } else {
	        TRACE2("getter asked to be called again in (ms) ", callMeBackIn_ms);
	    }
	    mNextAttempt = now + callMeBackIn_ms;
	}
    }
    return !callMeBack;
}


void Timestamp::toString(unsigned long now, Str *str) const
{
    unsigned long secondsSinceBoot = (now+500)/1000;
    unsigned long secondsSinceMark = secondsSinceBoot - secondsAtMark();
    unsigned long secondsSinceEpoch = secondsSinceMark + mTimestamp;

    char timestampStr[16];
    sprintf(timestampStr, "%lu", secondsSinceEpoch);

    *str = timestampStr;
}

