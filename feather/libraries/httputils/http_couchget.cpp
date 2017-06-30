#include <http_couchget.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>


static const char *DateTag = "Date: ";
static const char *ETag = "ETag: \"";


HttpCouchGet::HttpCouchGet(const Str &ssid, const Str &ssidPswd, 
			   const Str &host, int port, 
			   const Str &dbUser, const Str &dbPswd, 
			   bool isSSL, const char *urlPieces[])
  : HttpGet(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL, urlPieces),
    m_consumer(getContext())
{
  TF("HttpCouchGet::HttpCouchGet");
  init();
}


void HttpCouchGet::init()
{
    TF("HttpCouchGet::init");
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
    if (!m_parsedDoc) {

        if (m_consumer.hasOk()) {
	    HttpCouchGet *nonConstThis = (HttpCouchGet*) this;
	    nonConstThis->m_parsedDoc = true;
	    
	    nonConstThis->m_haveDoc = nonConstThis->m_consumer.parseDoc(&nonConstThis->m_doc) != 0;
	    nonConstThis->m_consumer.reset();
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


const StrBuf &HttpCouchGet::getETag() const {
    return m_consumer.getETag();
}


bool HttpCouchGet::getTimestamp(StrBuf *date) const {
    *date = m_consumer.getTimestamp();
    return true;
}


bool HttpCouchGet::hasTimestamp() const {
    TF("HttpCouchGet::hasTimestamp");
    return m_consumer.getTimestamp().len() > 0;
}
