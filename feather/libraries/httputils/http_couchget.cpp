#include <http_couchget.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>



HttpCouchGet::HttpCouchGet(const Str &ssid, const Str &ssidPswd, 
			   const Str &host, int port, const Str &page,
			   const Str &dbUser, const Str &dbPswd, 
			   bool isSSL)
  : HttpGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
    m_consumer(getContext())
{
  TF("HttpCouchGet::HttpCouchGet");
//PH2("entry; now: ", millis());  
  init();
}


void HttpCouchGet::init()
{
    TF("HttpCouchGet::init");
//PH2("entry; now: ", millis());  
    m_consumer.reset();
    m_doc.clear();
    m_haveDoc = m_parsedDoc = false;
}


void HttpCouchGet::resetForRetry()
{
    TF("HttpCouchGet::resetForRetry");
    init();

    HttpGet::resetForRetry();
}


bool HttpCouchGet::haveDoc() const
{
    TF("HttpCouchGet::haveDoc");
    TRACE("entry");
    if (!m_parsedDoc) {

        if (m_consumer.hasOk()) {
	    HttpCouchGet *nonConstThis = (HttpCouchGet*) this;
	    nonConstThis->m_parsedDoc = true;
	    
	    nonConstThis->m_haveDoc = nonConstThis->m_consumer.parseDoc(&nonConstThis->m_doc) != 0;
	    TRACE("parsed");
	}
    }
    return m_haveDoc;
}


bool HttpCouchGet::testSuccess() const
{
    TF("HttpCouchGet::testSuccess");
    bool r = getHeaderConsumer().hasOk() || getHeaderConsumer().hasNotFound();
    TRACE(r ? "looks good" : "failed");
    return r;
}
