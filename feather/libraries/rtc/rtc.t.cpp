#include <rtc.t.h>

#include <platformutils.h>
#include <couchutils.h>
#include <http_couchconsumer.h>

#include <http_couchget.h>

#define NDEBUG
#include <strutils.h>
#include <str.h>
#include <strbuf.h>

#include <Trace.h>

#include <rtcconversions.h>


static const char *TimestampDb = "test-persistent-enum";
static const char *TimestampDocId = "ok-doc";
static const char *DateTag = "Date: ";



RTCTest::~RTCTest() {}


class RTCTestGetter : public HttpCouchGet {
private:
    RTCTest *m_test;

public:
    RTCTestGetter(RTCTest *test,
		  const char *dbUser, const char *dbPswd,
		  const char *urlPieces[])
      : HttpCouchGet(test->ssid, test->pass, HttpOpTest::sslDbHost, HttpOpTest::sslDbPort,
		     dbUser, dbPswd, true, urlPieces),
	m_test(test)
    {
        TF("RTCTestGetter::RTCTestGetter");
	TRACE("entry");
    }
};


static void testTimestampConversions()
{
    Timestamp_t ts;
    assert(RTCConversions::cvtToTimestamp("Sat, 28 Jan 2017 22:24:18 GMT", &ts),
	   "Sat, 28 Jan 2017 22:24:18 GMT");
    assert(ts == 1485642258, "ts == 0");
}


bool RTCTest::loop()
{
    TF("RTCTest::loop");
    unsigned long now = millis();
    if (now > m_timeToAct && !m_didIt) {
	if (m_getter == NULL) {
	    TRACE("Testing cvtToTimestamp");
	    testTimestampConversions();
	    
	    TRACE("creating getter");
	    
	    static const char *urlPieces[5];
	    urlPieces[0] = "/";
	    urlPieces[1] = TimestampDb;
	    urlPieces[2] = urlPieces[0];
	    urlPieces[3] = TimestampDocId;
	    urlPieces[4] = 0;
	    
	    m_getter = m_rtcGetter = new RTCTestGetter(this, 
						       HttpOpTest::sslDbUser, HttpOpTest::sslDbPswd,
						       urlPieces);
	} else {
	    TRACE("processing event");
	    unsigned long callMeBackIn_ms = 0;
	    if (!m_getter->processEventResult(m_getter->event(now, &callMeBackIn_ms))) {
	        TRACE("done");
		StrBuf timestampStr;
		m_rtcGetter->getTimestamp(&timestampStr);
		TRACE2("TimestampStr: ", timestampStr.c_str());
		Timestamp_t ts;
		bool stat = RTCConversions::cvtToTimestamp(timestampStr.c_str(), &ts);
		if (stat) {
		    TRACE2("Timestamp: ", ts);
		    m_success = true;
		} else {
		    PL("Conversion to unix timestamp failed");
		    m_success = false;
		}
	        delete m_getter;
		m_getter = NULL;
	    }
	    m_timeToAct = now + callMeBackIn_ms;
	}
    }

    return m_success;
}
