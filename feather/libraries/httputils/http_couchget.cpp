#include <http_couchget.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>


HttpCouchGet::HttpCouchGet(const char *ssid, const char *ssidPswd, 
			   const char *host, int port, const char *page,
			   const char *credentials, bool isSSL)
  : HttpGet(ssid, ssidPswd, host, port, page, credentials, isSSL),
    m_consumer(getContext())
{
    init();
}


HttpCouchGet::HttpCouchGet(const char *ssid, const char *ssidPswd, 
			   const IPAddress &hostip, int port, const char *page,
			   const char *credentials, bool isSSL)
  : HttpGet(ssid, ssidPswd, hostip, port, page, credentials, isSSL),
    m_consumer(getContext())
{
    init();
}


HttpCouchGet::~HttpCouchGet()
{
}


void HttpCouchGet::init()
{
    m_consumer.reset();
    m_doc.clear();
    m_haveDoc = m_parsedDoc = false;
}


void HttpCouchGet::resetForRetry()
{
    TF("HttpCouchGet::resetForRetry");
    TRACE("entry");
    init();

    HttpGet::resetForRetry();
}


bool HttpCouchGet::haveDoc() const
{
    TF("HttpCouchGet::haveDoc");
    TRACE("entry");
    if (!m_parsedDoc) {
        HttpCouchGet *nonConstThis = (HttpCouchGet*) this;
	nonConstThis->m_haveDoc = nonConstThis->m_consumer.parseDoc(&nonConstThis->m_doc) != 0;
	nonConstThis->m_parsedDoc = true;
	TRACE("parsed");
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
