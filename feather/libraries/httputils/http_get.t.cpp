#include <http_get.t.h>

#include <http_couchget.h>

#define NDEBUG
#include <strutils.h>
#include <couchutils.h>

//#define INFINITE_LOOP

#include <str.h>


const char *HttpGetTest::expectedContent() const {
    return "Hi feather, if you can read this, the test worked!";
}


HttpGetTest::HttpGetTest(const char *dbUser, const char *dbPswd, 
			 const char *dbHost,
			 int dbPort,
			 bool isSSL)
  : HttpOpTest(dbUser, dbPswd, dbHost, dbPort, isSSL), m_getter(0)
{
}

HttpGetTest::HttpGetTest()
  : HttpOpTest(defaultDbUser, defaultDbPswd, defaultDbHost, defaultDbPort, false),
    m_getter(0)
{
}

HttpGetTest::~HttpGetTest()
{
    assert(m_getter == NULL, "m_getter hasn't been deleted");
}


bool HttpGetTest::loop() {
    unsigned long now = millis();
    if (now > m_timeToAct && !m_didIt) {
	if (m_getter == NULL) {
	    Str url;
	    CouchUtils::toURL(defaultDbName, defaultDocid, &url);
	    m_getter = new HttpCouchGet(ssid, pass, getDbHost(), getDbPort(),
					url.c_str(), getDbUser(), getDbPswd(), getIsSSL());
	} else {
	    unsigned long callMeBackIn_ms = 0;
	    if (!m_getter->processEventResult(m_getter->event(now, &callMeBackIn_ms))) {
		if (m_getter->haveDoc()) {
		    m_success = (strstr(m_getter->getHeaderConsumer().getResponse().c_str(),
					expectedContent()) != NULL);
		    if (m_success) {
		        PL("HttpGetTest::loop; Successfully retrieved the expected doc!");
			DL("HttpGetTest::loop; here it is: ");
#ifndef NDEBUG
			CouchUtils::printDoc(m_getter->getDoc());
#endif
		    } else {
		        PL("HttpGetTest::loop; received a doc with unexpected content:");
			CouchUtils::printDoc(m_getter->getDoc());
		    }
		} else {
		    PL("Don't have a valid doc");
		    m_success = false;
		}

	        delete m_getter;
		m_getter = NULL;
#ifndef INFINITE_LOOP		
		m_didIt = true;
#else
		callMeBackIn_ms = 10l;
		m_success = true;
#endif		
	    }
	    m_timeToAct = now + callMeBackIn_ms;
	}
    }

    return m_success;
}




