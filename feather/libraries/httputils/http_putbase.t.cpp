#include <http_putbase.t.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <http_couchget.h>

#include <str.h>


HttpPutBaseTest::HttpPutBaseTest(const char *dbUser, const char *dbPswd,
				 const char *dbHost,
				 int dbPort,
				 bool isSSL)
  : HttpOpTest(dbUser, dbPswd, dbHost, dbPort, isSSL), m_getter(0)
{
}

HttpPutBaseTest::~HttpPutBaseTest()
{
    assert(m_getter == NULL, "m_getter hasn't been deleted");
}


bool HttpPutBaseTest::loop() {
    TF("HttpPutBaseTest::loop");
    
    unsigned long now = millis();
    if (now > m_timeToAct && !m_didIt) {
        if (m_getter == NULL) {
	    TRACE("creating getter");
	    Str url;
	    CouchUtils::toURL(defaultDbName, getDocid(), &url);
	    m_getter = new HttpCouchGet(ssid, pass, getDbHost(), getDbPort(),
					url.c_str(), getDbUser(), getDbPswd(), getIsSSL());
	} else {
	    unsigned long callMeBackIn_ms = 0;
	    HttpCouchGet::EventResult r = m_getter->event(now, &callMeBackIn_ms);
	    if (!m_getter->processEventResult(r)) {
	        TRACE("done processing getter events");
		bool failed = true;
		if (m_getter->haveDoc()) {
		    CouchUtils::Doc copyOfDoc(m_getter->getDoc());
		    delete m_getter; // getter must be destructed before putter is created
		    failed = !createPutter(copyOfDoc);
		} else {
		    delete m_getter;
		}
		m_getter = NULL;
		if (failed) {
		    m_didIt = true;
		    m_success = false;
		}
		callMeBackIn_ms = 10l;
	    }
	    m_timeToAct = now + callMeBackIn_ms;
	}
    }

    return m_success;
}
