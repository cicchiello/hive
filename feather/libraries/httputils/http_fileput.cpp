#include <http_fileput.h>

#include <wifiutils.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>
#include <sdutils.h>

#include <MyWiFi.h>

#include <str.h>

#include <SPI.h>
#include <SdFat.h>

#include <platformutils.h>

HttpFilePut::HttpFilePut(const char *ssid, const char *ssidPswd, 
			 const char *host, int port, 
			 const char *dbUser, const char *dbPswd,
			 bool isSSL, const char *filename, const char *contentType,
			 const char *urlPieces[])
  : HttpCouchGet(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL, urlPieces),
    m_filename(filename), m_contentType(contentType), mRetryFlush(false)
{
    TF("HttpBinaryPut::HttpBinaryPut");

    init();
   
    TRACE2("Using ssid: ",ssid);
    TRACE2("Using host: ",host);
    TRACE2("Using port: ",port);
    TRACE2("Using page: ",page);
    TRACE2("Using dbUser: ", (dbUser?dbUser:"<null>"));
    TRACE2("Using dbPswd: ", (dbPswd?dbPswd:"<null>"));
    TRACE2("Using contentType: ", contentType);
}


void HttpFilePut::init()
{
    m_sd = NULL;
    m_f = NULL;
    m_writtenCnt = 0;
}


HttpFilePut::~HttpFilePut()
{
    assert(m_f == NULL, "m_f wasn't deleted");
    assert(m_sd == NULL, "m_sd wasn't deleted");
}


void HttpFilePut::resetForRetry()
{
    TF("HttpFilePut::resetForRetry");
    TRACE("entry");
    init();
    HttpCouchGet::resetForRetry();
}


void HttpFilePut::sendPUT(Stream &s, int contentLength) const
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


int HttpFilePut::getContentLength() const
{
    return m_f->fileSize();
}

HttpFilePut::EventResult HttpFilePut::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpFilePut::event; ");
    
    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TRACE("ISSUE_OP");

	sendPUT(getContext().getClient(), getContentLength());
	
	bool done = true; // most paths are terminal -- so make the exceptions be explicit
	m_sd = new SdFat();
	if (SDUtils::initSd(*m_sd)) {
	    if (m_sd->exists(m_filename.c_str())) {
	        TRACE("sd initialized");
		m_f = new SdFile();
		m_writtenCnt = 0;
		if (!m_f->open(m_filename.c_str(), O_READ)) {
		    TRACE2("Couldn't open file ", m_filename.c_str());
		} else {
		    TRACE2("Uploading ", m_filename.c_str());
		    TRACE3("  (", m_f->fileSize(), " bytes) ...");

		    sendPUT(getContext().getClient(), m_f->fileSize());
		    
		    setOpState(ISSUE_OP_FLUSH);
		    getHeaderConsumer().setTimeout(20000); // 20s

		    *callMeBackIn_ms = 1l;
		    done = false;
		}
	    } else {
	        TRACE3("file ", m_filename.c_str(), " doesn't exist");
	    }
	} else {
	    TRACE("SDUtils::initSd failed");
	}
	
	if (done) {
	    int remaining;
	    getContext().getClient().flushOut(&remaining);
	    
	    setOpState(DISCONNECTED);
	    setFinalResult(IssueOpFailed);
	    
	    delete m_f;
	    delete m_sd;
	    
	    m_f = NULL;
	    m_sd = NULL;
	    return getFinalResult();
	}
	return CallMeBack;
    }
      break;
    case CHUNKING: {
        if (mRetryFlush) {
	    int remaining;
	    getContext().getClient().flushOut(&remaining);
	    mRetryFlush = remaining > 0;
	} else {
	    char buf[512];
	    int numread = m_f->read(buf, 512);
	    if (numread > 0) {
	        m_writtenCnt += getContext().getClient().write(buf, numread);
	    
		int remaining;
		getContext().getClient().flushOut(&remaining);
		mRetryFlush = remaining > 0;
	    } else {
	        TRACE("Nothing read!?!?!");
	    }
	    if (numread < 512) {
	        TRACE3("Done writing; wrote ", m_writtenCnt, " bytes");
		setOpState(CONSUME_RESPONSE);
		if (m_writtenCnt != m_f->fileSize()) {
		    TRACE("m_writtenCnt doesn't match m_f->fileSize()");
		    TRACE2("m_writtenCnt == ", m_writtenCnt);
		    TRACE2("m_f->fileSize() == ", m_f->fileSize());
		}
		assert(m_writtenCnt == m_f->fileSize(), "m_writtenCnt == m_f->fileSize()");
		m_f->close();
		delete m_f;
		delete m_sd;
		m_f = NULL;
		m_sd = NULL;
	    }
	}
	*callMeBackIn_ms = 10l;
	return CallMeBack;
    }
      break;
    default:
        return this->HttpCouchGet::event(now, callMeBackIn_ms);
    }
}


