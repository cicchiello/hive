#include <http_binaryput.h>

#include <wifiutils.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <MyWiFi.h>

#include <http_dataprovider.h>

#include <platformutils.h>


//#define ENABLE_TRACKING
#ifdef ENABLE_TRACKING
struct Tracker {
  unsigned long writeDelta;
  unsigned long endTime;
};
static int trackerCnt = 0;
static Tracker tracker[1000];


void dumpStats()
{
    P(0); P(","); P(tracker[0].writeDelta); P(","); PL(tracker[0].endTime);
    unsigned long maxDelta = 0, totalDelta = 0;
    for (int i = 1; i < trackerCnt; i++) {
      if (tracker[i].writeDelta > maxDelta)
	maxDelta = tracker[i].writeDelta;
      totalDelta += tracker[i].writeDelta;
      P(i); P(","); P(tracker[i].writeDelta); P(","); PL(tracker[i].endTime-tracker[i-1].endTime);
    }
    P("maxDelta: "); PL(maxDelta);
    P("avg delta: "); PL(totalDelta/trackerCnt);
    trackerCnt = 0;
}
#endif


void HttpBinaryPut::resetForRetry()
{
    TF("HttpBinaryPut::resetForRetry");
    TRACE("entry");
    m_provider->reset();
    m_writtenCnt = 0;
    HttpCouchGet::resetForRetry();
}


void HttpBinaryPut::sendPUT(Stream &s, int contentLength) const
{
    TF("HttpBinaryPut::sendPUT");
    P("PUT ");
    s.print("PUT ");
    sendPage(s);
    P(" ");
    s.print(" ");
    PL(TAGHTTP11);
    s.println(TAGHTTP11);
    PL("User-Agent: Hive/0.0.1");
    s.println("User-Agent: Hive/0.0.1");
    sendHost(s);
    PL("Accept: */*");
    s.println("Accept: */*");
    P("Content-Type: ");
    s.print("Content-Type: ");
    PL(m_contentType.c_str());
    s.println(m_contentType.c_str());
    P("Content-Length: ");
    PL(contentLength);
    s.print("Content-Length: ");
    s.println(contentLength);
    PL("Conection: close");
    s.println("Connection: close");
    PL("");
    s.println();
}


HttpBinaryPut::EventResult HttpBinaryPut::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpBinaryPut::event");
    //TRACE("entry");
    
    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TF("HttpBinaryPut::event; ISSUE_OP");

	sendPUT(getContext().getClient(), m_provider->getSize());
	setOpState(ISSUE_OP_FLUSH);
	m_provider->start();
	getHeaderConsumer().setTimeout(20000); // 20s
	
	*callMeBackIn_ms = 1l;
	return CallMeBack;
    }
      break;
      
    case ISSUE_OP_FLUSH: {
        TF("HttpBinaryPut::event; ISSUE_OP_FLUSH");
        EventResult er = HttpCouchGet::event(now, callMeBackIn_ms);
        if (getOpState() != ISSUE_OP_FLUSH)
	    setOpState(CHUNKING);
	return CallMeBack;
    }
      break;
      
    case CHUNKING: {
        TF("HttpBinaryPut::event; CHUNKING");
        if (mRetryFlush) {
	    int remaining;
	    getContext().getClient().flushOut(&remaining);
	    mRetryFlush = remaining > 0;
	} else {
	    if (m_provider->isDone()) {
	        TRACE3("Done writing; wrote: ", m_writtenCnt, " bytes");
		setOpState(CONSUME_RESPONSE);
		if (m_writtenCnt != m_provider->getSize()) {
		    TRACE("m_writtenCnt doesn't match m_provider->getSize()");
		    TRACE2("m_writtenCnt == ", m_writtenCnt);
		    TRACE2("m_provider->getSize() == ", m_provider->getSize());
		}
		assert(m_writtenCnt == m_provider->getSize(), "m_writtenCnt == m_provider->getSize()");
		m_provider->close();
	    } else {
	        unsigned char *buf = 0;
		int sz = m_provider->takeIfAvailable(&buf);
		if (sz > 0) {
#ifdef ENABLE_TRACKING		
		    unsigned long b = micros();
#endif
		    TRACE2("Sending a chunk of size ", sz);
                    getContext().getClient().write(buf, sz);

		    int remaining;
		    getContext().getClient().flushOut(&remaining);
		    mRetryFlush = remaining > 0;

		    if (mRetryFlush) {
		        TRACE2("a retry is necessary: ", remaining);
		    }

#ifndef HEADLESS
#ifndef NDEBUG
		    TRACE("Sent the following chunk: ");
		    for (int ii = 0; ii < sz; ii++) {
		        _TAG("TRACE"); _TAG(tscope.getFunc()); _TAG(__FILE__); _TAG(__LINE__);
			Serial.println((int) buf[ii], HEX);
		    }
#endif
#endif

		    m_provider->giveBack(buf);
		    m_writtenCnt += sz;

#ifdef ENABLE_TRACKING		
		    unsigned long n = micros();
		    tracker[trackerCnt].endTime = n;
		    tracker[trackerCnt++].writeDelta = n - b;
#endif
		} else {
		    TRACE("Nothing available");
		}
	    }
	}

	*callMeBackIn_ms = 1l;
	return CallMeBack;
    }
      break;
    default:
        TF("HttpBinaryPut::event; default");
        return this->HttpCouchGet::event(now, callMeBackIn_ms);
    }
}


