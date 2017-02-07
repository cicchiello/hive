#include <http_couchconsumer.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <MyWiFi.h>


HttpCouchConsumer::HttpCouchConsumer(const WifiUtils::Context &ctxt)
  : HttpHeaderConsumer(ctxt)
{
    TF("HttpCouchConsumer::HttpCouchConsumer");
    TRACE("entry");
    init();
}


HttpCouchConsumer::~HttpCouchConsumer()
{
    DL("HttpCouchConsumer; DTOR");
}


void HttpCouchConsumer::init()
{
    m_contentCnt = 0;
}


void HttpCouchConsumer::reset()
{
    init();
    this->HttpHeaderConsumer::reset();
}


bool HttpCouchConsumer::consume(unsigned long now)
{
    TF("HttpCouchConsumer::consume");
    if (HttpHeaderConsumer::consume(now)) {
        return true;
    } else {
        TRACE("consuming the couch part of the header response");
	Adafruit_WINC1500Client &client = m_ctxt.getClient();
	if (client.connected()) {
	    TRACE("consuming couch response document");
	    int avail;
	    if (avail = client.available()) {
	        int read = -1;
	        if (!isError() && hasOk()) {
		    TRACE("no error and hasOk");
		    m_response.expand(m_response.len()+avail+1);
		    uint8_t *s = (uint8_t*) m_response.c_str();
		    int l = m_response.len();
		    read = client.read(&s[l], avail);
		    if (read != avail) {
		        PL("HttpCouchConsumer::consume; error while consuming "
			   "couch response document");
			return false; // indicate done consuming
		    }
#ifndef NDEBUG
		    for (int i = 0; i < avail; i++) {
		        char c = s[l+i];
			assert(c, "Unexpected NULL char found in http response stream");
		    }
#endif
		    TRACE3("response so far: <<", m_response.c_str(), ">>");
		} else {
		    TRACE("response is error");
		    // silently consume
		    uint8_t buf[33];
		    int a = avail;
		    if (a > 32) a = 32;
		    read = client.read(buf, a);
		    assert(avail-read == client.available(), "avail-read == client.available()");
		    buf[read] = 0;
		}
		m_contentCnt += read;
	    } else if (now - consumeStart() > getTimeout()) {
	        PL("HttpCouchConsumer::consume; consume timeout exceeded");
		return false; // indicate done consuming
	    }
	} else {
	    assert(isError() || hasOk(), "isError() || hasOk()");
	    return false; // indicate done consuming
	}
	return true; // indicate continue consuming
    }
}


const char *HttpCouchConsumer::parseDoc(CouchUtils::Doc *doc)
{
    DL("HttpCouchConsumer::parseDoc");
    return CouchUtils::parseDoc(getResponse().c_str(), doc);
}
