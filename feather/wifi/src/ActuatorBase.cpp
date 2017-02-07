#include <ActuatorBase.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <hiveconfig.h>

#include <str.h>

#include <http_couchget.h>


ActuatorBase::ActuatorBase(const HiveConfig &config, const char *name, unsigned long now)
  : Actuator(name, now+10*1000), mNextActionTime(now+10000l),
    mConfig(config), mGetter(NULL)
{
    TF("ActuatorBase::ActuatorBase");
}


/* STATIC */
void ActuatorBase::setNextTime(unsigned long now, unsigned long *t)
{
    *t = now + 30000l /*rateProvider.secondsBetweenSamples()*1000l*/;
}


bool ActuatorBase::isItTimeYet(unsigned long now) const
{
    return (now >= mNextActionTime);
}


bool ActuatorBase::loop(unsigned long now, Mutex *wifi)
{
    TF("ActuatorBase::loop");
    if (wifi->own(getSemaphore())) {
        if (mGetter == NULL) {
	    TRACE("creating getter");

	    mGetter = createGetter();
	    mNextActionTime = now + 10l;
	} else {
	    TRACE("processing event");
	    unsigned long callMeBackIn_ms = 0;
	    if (!mGetter->processEventResult(mGetter->event(now, &callMeBackIn_ms))) {
	        TRACE("done");
		if (mGetter->hasResult()) {
		    processResult(mGetter);
		    setNextTime(now, &mNextActionTime);
		} else {
		    TRACE("invalid response; retrying again in 5s");
		    mNextActionTime = now + 5000l;
		}
		delete mGetter;
		mGetter = NULL;
		wifi->release(getSemaphore());
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
			     const char *url, const char *credentials, bool isSSL)
  : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, credentials, isSSL)
{
    TF("ActuatorBase::Getter::Getter");
}


const CouchUtils::Doc *ActuatorBase::Getter::getSingleRecord(CouchUtils::Doc *doc) const
{
    TF("ActuatorBase::Getter:getSingleRecord");
    if (m_consumer.hasOk()) {
        TRACE(m_consumer.getResponse().c_str());

	const char *jsonStr = strstr(m_consumer.getResponse().c_str(), "total_rows");
	if (jsonStr != NULL) {
	    ActuatorBase::Getter *nonConstThis = (ActuatorBase::Getter*)this;
	    // work back to the beginning of the doc... then some ugly parsing!
	    while ((jsonStr > m_consumer.getResponse().c_str()) && (*jsonStr != '{'))
	        --jsonStr;

	    CouchUtils::Doc doc;
	    const char *remainder = CouchUtils::parseDoc(jsonStr, &doc);
	    if (remainder != NULL) {
	        int ir = doc.lookup("rows");
		if ((ir>=0) && doc[ir].getValue().isArr()) {
		    const CouchUtils::Arr &rows = doc[ir].getValue().getArr();
		    if ((rows.getSz() == 1) && rows[0].isDoc()) {
		        const CouchUtils::Doc &record = rows[0].getDoc();
			return &record;
		    }
		}
	    }
	}
    }
    return NULL;
}

