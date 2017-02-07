#include <http_put.t.h>

#define NDEBUG
#include <strutils.h>

#include <http_couchput.h>


#include <couchutils.h>


//
// Most code developed using Adafruit_WINC1500/examples/WifiWebClient.ino as reference
//


HttpPutTest::HttpPutTest(const char *credentials, const char *dbHost, int dbPort, bool isSSL)
  : HttpPutBaseTest(credentials, dbHost, dbPort, isSSL), m_putter(0)
{
}

HttpPutTest::HttpPutTest()
  : HttpPutBaseTest(defaultDbCredentials, defaultDbHost, defaultDbPort, false),
    m_putter(0)
{
}


HttpPutTest::~HttpPutTest()
{
    assert(m_putter == NULL, "m_putter hasn't been deleted");
}


const char *HttpPutTest::getDocid() const
{
    return defaultDocid;
}


bool HttpPutTest::createPutter(const CouchUtils::Doc &originalDoc)
{
    DL("HttpPutTest::createPutter");
    
#ifndef NDEBUG    
    PHL("Here's the doc that I'll be updating:");
    CouchUtils::printDoc(originalDoc);
#endif
    
    int irev = originalDoc.lookup("_rev");
    int icontent = originalDoc.lookup("content");
    int att = originalDoc.lookup("_attachments");
    if ((irev >= 0) && (icontent >= 0)) {
        CouchUtils::Doc updateDoc;
        const Str &revision = originalDoc[irev].getValue().getStr();
	const Str &content = originalDoc[icontent].getValue().getStr();
	updateDoc.addNameValue(new CouchUtils::NameValuePair("_rev", revision));
	updateDoc.addNameValue(new CouchUtils::NameValuePair("content2", "Hi CouchDB!"));
	updateDoc.addNameValue(new CouchUtils::NameValuePair("content", content));
//	if (att >= 0) 
//          updateDoc.addNameValue(new CouchUtils::Doc::NameValuePair(originalDoc[att]));
    
#ifndef NDEBUG
	PHL("Here's the updated doc:");
	CouchUtils::printDoc(updateDoc);
#endif

	Str url;
	CouchUtils::toURL(defaultDbName, getDocid(), &url);

	m_putter = new HttpCouchPut(ssid, pass, getDbHost(), getDbPort(),
				    url.c_str(), updateDoc, getCredentials(), getIsSSL());

	return m_putter != NULL;
    } else {
        PHL("Invalid doc encountered:");
	CouchUtils::printDoc(originalDoc);
        return false;
    }



    CouchUtils::Doc updateDoc;
    if (createUpdateDoc(m_getter->getDoc(), &updateDoc));
    



    Str url;
    CouchUtils::toURL(defaultDbName, getDocid(), &url);

    m_putter = new HttpCouchPut(ssid, pass, getDbHost(), getDbPort(),
				url.c_str(), updateDoc, getCredentials(), getIsSSL());

    return m_putter != NULL;
}


bool HttpPutTest::loop() {
    PF("HttpPutTest::loop; ");
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
		        PHL("doc successfully updated!");
			m_success = true;
		    } else {
		        PHL("Found unexpected information in the doc:");
			CouchUtils::printDoc(m_putter->getDoc());
		    }
		} else {
		    PHL("Found unexpected information in the doc:");
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


