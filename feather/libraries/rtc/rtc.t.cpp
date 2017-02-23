#include <rtc.t.h>

#include <platformutils.h>
#include <couchutils.h>
#include <http_couchconsumer.h>

#include <http_couchget.h>

#define NDEBUG
#include <strutils.h>
#include <str.h>

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
		  const char *url, const char *dbUser, const char *dbPswd)
      : HttpCouchGet(test->ssid, test->pass, HttpOpTest::sslDbHost, HttpOpTest::sslDbPort,
		     url, dbUser, dbPswd, true),
	m_test(test)
    {
        TF("RTCTestGetter::RTCTestGetter");
	TRACE("entry");
    }
 
    Str getTimestamp() const {
        TF("RTCTestGetter; getTimestamp");
	TRACE("entry");
        const char *dateStr = strstr(m_consumer.getResponse().c_str(), DateTag);
	if (dateStr != NULL) {
	    dateStr += strlen(DateTag);
	    Str date;
	    while (*dateStr != 13) date.add(*dateStr++);
	    TRACE("Received timestamp: "); PL(date.c_str());
	    return date;
	} else {
	    return Str("unknown");
	}
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
	    Str url;
	    CouchUtils::toURL(TimestampDb, TimestampDocId, &url);
	    m_getter = m_rtcGetter = new RTCTestGetter(this, url.c_str(),
						       HttpOpTest::sslDbUser, HttpOpTest::sslDbPswd);
	} else {
	    TRACE("processing event");
	    unsigned long callMeBackIn_ms = 0;
	    if (!m_getter->processEventResult(m_getter->event(now, &callMeBackIn_ms))) {
	        TRACE("done");
		Str timestampStr = m_rtcGetter->getTimestamp();
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
