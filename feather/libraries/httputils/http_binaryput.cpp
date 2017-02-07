#include <http_binaryput.h>

#include <wifiutils.h>

//#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <MyWiFi.h>

#include <http_dataprovider.h>

#include <platformutils.h>


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



HttpBinaryPut::HttpBinaryPut(const char *ssid, const char *ssidPswd, 
			     const char *host, int port, const char *page, const char *credentials,
			     bool isSSL, HttpDataProvider *provider, const char *contentType)
  : HttpCouchGet(ssid, ssidPswd, host, port, page, credentials, isSSL),
    m_provider(provider), m_contentType(contentType), m_writtenCnt(0)
{
   TF("HttpBinaryPut::HttpBinaryPut");
   TRACE2("Using ssid: ",ssid);
   TRACE2("Using host: ",host);
   TRACE2("Using port: ",port);
   TRACE2("Using page: ",page);
   TRACE2("Using credentials: ", (credentials?credentials:"<null>"));
   TRACE2("Using contentType: ", contentType);
}

HttpBinaryPut::HttpBinaryPut(const char *ssid, const char *ssidPswd, 
			     const IPAddress &hostip, int port, const char *page, const char *credentials,
			     bool isSSL, HttpDataProvider *provider, const char *contentType)
  : HttpCouchGet(ssid, ssidPswd, hostip, port, page, credentials, isSSL),
    m_provider(provider), m_contentType(contentType), m_writtenCnt(0)
{
   TF("HttpBinaryPut::HttpBinaryPut");
   TRACE2("Using ssid: ",ssid);
   uint32_t a = hostip;
   TRACE2("Using host: ",a);
   TRACE2("Using port: ",port);
   TRACE2("Using page: ",page);
   TRACE2("Using contentType: ", contentType);
}


HttpBinaryPut::~HttpBinaryPut()
{
}


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


static unsigned long lastTime = 0;
HttpBinaryPut::EventResult HttpBinaryPut::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpBinaryPut::event");
    //TRACE("entry");
    
    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TRACE("ISSUE_OP");

	sendPUT(getContext().getClient(), m_provider->getSize());
	setOpState(CHUNKING);
	m_provider->start();
	getHeaderConsumer().setTimeout(20000); // 20s
	
	*callMeBackIn_ms = 1l;
	return CallMeBack;
    }
      break;
    case CHUNKING: {
        unsigned char *buf = 0;
	int sz = m_provider->takeIfAvailable(&buf);
	while (sz > 0) {
	    unsigned long b = micros();
	    getContext().getClient().write(buf, sz);
	    m_provider->giveBack(buf);
	    m_writtenCnt += sz;
	    unsigned long n = micros();
	    tracker[trackerCnt].endTime = n;
	    tracker[trackerCnt++].writeDelta = n - b;

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
		sz = 0;
	    } else {
	        PlatformUtils::nonConstSingleton().clearWDT();
	        sz = m_provider->takeIfAvailable(&buf);
	    }
	}
      
	*callMeBackIn_ms = 1l;
	return CallMeBack;
    }
      break;
    default:
        return this->HttpCouchGet::event(now, callMeBackIn_ms);
    }
}


