#include <ActuatorBase.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <hiveconfig.h>

#include <str.h>
#include <strbuf.h>

#include <http_couchget.h>


ActuatorBase::ActuatorBase(const HiveConfig &config, const RateProvider &rateProvider,
			   const char *name, unsigned long now)
  : Actuator(name, now+10*1000), mNextActionTime(now+10000l),
    mConfig(config), mRateProvider(rateProvider), mGetter(NULL)
{
    TF("ActuatorBase::ActuatorBase");
}


/* STATIC */
void ActuatorBase::setNextTime(unsigned long now, unsigned long *t)
{
    *t = now + 30000l /*mRateProvider.secondsBetweenSamples()*1000l*/;
}


bool ActuatorBase::loop(unsigned long now, Mutex *wifi)
{
    TF("ActuatorBase::loop");
    if (wifi->own(this)) {
        if (mGetter == NULL) {
	    TRACE("creating getter");

	    mGetter = createGetter();
	    mNextActionTime = now + 10l;
	} else {
	    //TRACE("processing event");
	    unsigned long callMeBackIn_ms = 0;
	    if (!mGetter->processEventResult(mGetter->event(now, &callMeBackIn_ms))) {
	        TRACE("done");
		bool retry = true;
		if (mGetter->hasResult()) {
		    TRACE("have result");
		    retry = false;
		    processResult(mGetter);
		    setNextTime(now, &mNextActionTime);
		} else if (mGetter->isError()) {
		    TRACE("invalid response; retrying again in 5s");
		    mNextActionTime = now + 5000l;
		} else if (mGetter->isTimeout()) {
		    TRACE("timeout; GET failed");
		    mNextActionTime = now + 5000l;
		} else {
		    TRACE("No record found; using defaults");
		    setNextTime(now, &mNextActionTime);
		}
		mGetter->shutdownWifiOnDestruction(retry);
		delete mGetter;
		mGetter = NULL;
		wifi->release(this);
	    } else {
	        mNextActionTime = now + callMeBackIn_ms;
	    }
	}
    } else {
        mNextActionTime = now + 10l;
    }
      
    
    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}


ActuatorBase::Getter::Getter(const char *ssid, const char *pswd,
			     const char *dbHost, int dbPort,
			     const char *dbUser, const char *dbPswd, bool isSSL,
			     const char *urlPieces[])
  : HttpCouchGet(ssid, pswd, dbHost, dbPort, dbUser, dbPswd, isSSL, urlPieces),
    mIsParsed(false), mIsError(false), mRecord(0), mIsValueParsed(false), mValue(0)
{
    TF("ActuatorBase::Getter::Getter");
}


bool ActuatorBase::Getter::isError() const
{
    TF("ActuatorBase::Getter::isError");
    TRACE2("mIsError: ", mIsError);
    TRACE2("m_consumer.isError(): ", m_consumer.isError());
    return mIsError || m_consumer.isError();
}


const CouchUtils::Doc *ActuatorBase::Getter::getSingleRecord() const
{
    if (mIsParsed)
        return mRecord;
  
    TF("ActuatorBase::Getter:getSingleRecord");
    Getter *nonConstThis = (Getter*) this;
    nonConstThis->mIsParsed = true;
    nonConstThis->mIsError = true;
    if (m_consumer.hasOk()) {
	const char *jsonStr = strstr(m_consumer.getContent().c_str(), "total_rows");
	if (jsonStr != NULL) {
	    // work back to the beginning of the doc... then some ugly parsing!
	    while ((jsonStr > m_consumer.getContent().c_str()) && (*jsonStr != '{'))
	        --jsonStr;

	    CouchUtils::Doc doc;
	    const char *remainder = CouchUtils::parseDoc(jsonStr, &doc);
	    
	    if (remainder != NULL) {
	        int ir = doc.lookup("rows");
		if ((ir>=0) && doc[ir].getValue().isArr()) {
		    const CouchUtils::Arr &rows = doc[ir].getValue().getArr();
		    TRACE2("rows.getSz(): ", rows.getSz());
		    nonConstThis->mIsError =
		        !((rows.getSz() == 0) || ((rows.getSz() == 1) && rows[0].isDoc()));
		    TRACE2("mIsError: ", mIsError);
		    if (!mIsError && (rows.getSz() == 1)) {
		        nonConstThis->mRecord = &rows[0].getDoc();
			return mRecord;
		    }
		}
	    }
	}
    }
    return NULL;
}


const Str *ActuatorBase::Getter::getSingleValue() const
{
    if (mIsValueParsed)
        return mValue;

    Getter *nonConstThis = (Getter*) this;
    nonConstThis->mIsValueParsed = true;
    nonConstThis->mValue = 0;
	
    const CouchUtils::Doc *record = getSingleRecord();
    if (record != NULL) {
        nonConstThis->mIsError = true;
	int ival = record->lookup("value");
	if ((ival >= 0) &&
	    (*record)[ival].getValue().isArr() &&
	    ((*record)[ival].getValue().getArr().getSz()==4)) {
	    const CouchUtils::Arr &val = (*record)[ival].getValue().getArr();
	    if (!val[3].isDoc() && !val[3].isArr()) {
	        const Str &locStr = val[3].getStr();
		nonConstThis->mValue = &locStr;
		nonConstThis->mIsError = false;
		return mValue;
	    }
	}
    }

    return 0;
}


