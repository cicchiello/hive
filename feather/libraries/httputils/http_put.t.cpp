#include <http_put.t.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <http_couchput.h>


#include <couchutils.h>


//
// Most code developed using Adafruit_WINC1500/examples/WifiWebClient.ino as reference
//


HttpPutTest::HttpPutTest(const char *dbUser, const char *dbPswd, const char *dbHost, int dbPort, bool isSSL)
  : HttpPutBaseTest(dbUser, dbPswd, dbHost, dbPort, isSSL), m_putter(0)
{
}

HttpPutTest::HttpPutTest()
  : HttpPutBaseTest(defaultDbUser, defaultDbPswd, defaultDbHost, defaultDbPort, false),
    m_putter(0)
{
}


HttpPutTest::~HttpPutTest()
{
    TF("HttpPutTest::~HttpPutTest");
    assert(m_putter == NULL, "m_putter hasn't been deleted");
}


const char *HttpPutTest::getDocid() const
{
    return defaultDocid;
}


bool HttpPutTest::createPutter(const CouchUtils::Doc &originalDoc)
{
    TF("HttpPutTest::createPutter");
    
#ifndef NDEBUG    
    PH("Here's the doc that I'll be updating:");
    CouchUtils::printDoc(originalDoc);
#endif
    
    int irev = originalDoc.lookup("_rev");
    int icontent = originalDoc.lookup("content");
    int att = originalDoc.lookup("_attachments");
    if ((irev >= 0) && (icontent >= 0)) {
        CouchUtils::Doc *updateDoc = new CouchUtils::Doc();
        const Str &revision = originalDoc[irev].getValue().getStr();
	const Str &content = originalDoc[icontent].getValue().getStr();
	updateDoc->addNameValue(new CouchUtils::NameValuePair("_rev", revision));
	updateDoc->addNameValue(new CouchUtils::NameValuePair("content2", "Hi CouchDB!"));
	updateDoc->addNameValue(new CouchUtils::NameValuePair("content", content));
//	if (att >= 0) 
//          updateDoc->addNameValue(new CouchUtils::Doc::NameValuePair(originalDoc[att]));
    
#ifndef NDEBUG
	PH("Here's the updated doc:");
	CouchUtils::printDoc(*updateDoc);
#endif

	static const char *urlPieces[3];
	urlPieces[0] = "/";
	urlPieces[1] = defaultDbName;
	urlPieces[2] = urlPieces[0];
	urlPieces[3] = getDocid();
	urlPieces[4] = 0;

	m_putter = new HttpCouchPut(ssid, pass, getDbHost(), getDbPort(),
				    updateDoc, getDbUser(), getDbPswd(), getIsSSL(), urlPieces);

	return m_putter != NULL;
    } else {
        PH("Invalid doc encountered:");
	CouchUtils::printDoc(originalDoc);
        return false;
    }
}


bool HttpPutTest::loop() {
    TF("HttpPutTest::loop");
    unsigned long now = millis();
    if (now > m_timeToAct && !m_didIt) {
        if (m_putter == NULL) {
	    return this->HttpPutBaseTest::loop();
	} else {
            unsigned long callMeBackIn_ms = 0;
            HttpCouchPut::EventResult r = m_putter->event(now, &callMeBackIn_ms);
	    if (!m_putter->processEventResult(r)) {
	        m_success = false;
	        if (m_putter->haveDoc()) {
		    if (m_putter->getDoc().lookup("ok") >= 0) {
		        PH("doc successfully updated!");
			m_success = true;
		    } else {
		        PH("Found unexpected information in the doc:");
			CouchUtils::printDoc(m_putter->getDoc());
		    }
		} else {
		    PH("Found unexpected information in the doc:");
		    PL(m_putter->getHeaderConsumer().getResponse().c_str());
		}
		m_didIt = true;
		delete m_putter;
		m_putter = NULL;
	    }
	    m_timeToAct = now + callMeBackIn_ms;
	}
    }

    return m_success;
}


