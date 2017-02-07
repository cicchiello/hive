#include <http_headerconsumer.h>

#include <Arduino.h>

#define NDEBUG

#include <Trace.h>

#include <strutils.h>

#include <MyWiFi.h>


/* STATIC */
const char *HttpHeaderConsumer::TAGContentLength = "Content-Length:";
const char *HttpHeaderConsumer::TAGTransferEncoding = "Transfer-Encoding:";
const char *HttpHeaderConsumer::TAGChunked = "chunked";
const char *HttpHeaderConsumer::TAG200 = "HTTP/1.1 200 OK";
const char *HttpHeaderConsumer::TAG201 = "HTTP/1.1 201 Created";


HttpHeaderConsumer::HttpHeaderConsumer(const WifiUtils::Context &ctxt)
  : HttpResponseConsumer(ctxt)
{
    TF("HttpHeaderConsumer::HttpHeaderConsumer");
    TRACE("entry");
    init();
}


void HttpHeaderConsumer::init()
{
    m_timedout = m_haveHeader = m_parsedDoc = m_haveFirstCRLF = m_gotCR = m_err = false;
    mIsChunked = mHasContentLength = false;
    m_firstConsume = true;
    m_contentLength = 0;
    m_firstConsumeTime = 0;
    m_response.clear();
}


void HttpHeaderConsumer::reset()
{
    init();
    this->HttpResponseConsumer::reset();
}


bool HttpHeaderConsumer::hasOk() const
{
    TF("HttpHeaderConsumer::hasOk");
    if (m_parsedDoc) {
        TRACE("already parsed");
        return !m_err;
    }
    
    if (!m_err) {
        HttpHeaderConsumer *nonConstThis = (HttpHeaderConsumer*) this;
        nonConstThis->m_parsedDoc = nonConstThis->m_err = true;
        if ((strstr(m_response.c_str(), TAG200) != NULL) ||
	    (strstr(m_response.c_str(), TAG201) != NULL)) {
	    const char *cl = strstr(m_response.c_str(), TAGContentLength);
	    if (cl != NULL) {
		cl = StringUtils::eatWhitespace(cl + strlen(TAGContentLength));
		if (cl != NULL) {
		    TRACE("found ok and Content-Length");
		    HttpHeaderConsumer *nonConstThis = (HttpHeaderConsumer *) this;
		    nonConstThis->m_contentLength = atoi(cl);
		    nonConstThis->m_err = false;
		    nonConstThis->mHasContentLength = true;
TRACE("returning true");
		    return true;
		} else {
		    TRACE("Couldn't parse the Content-Length from HTTP response");
		}
	    } else {
	        const char *te = strstr(m_response.c_str(), TAGTransferEncoding);
	        if ((te != NULL) && strstr(te, TAGChunked)) {
		    TRACE("found ok and Chunked");
		    nonConstThis->mIsChunked = true;
		    nonConstThis->m_err = false;
		    return true;
		} else {
		    TRACE("Couldn't extract the Content-Length from HTTP response");
		}
	    }
	} else {
	    TRACE("HTTP failure response received: ");
	}
	TRACE(m_response.c_str());
    }
    return false;
}

bool HttpHeaderConsumer::consume(unsigned long now)
{
    TF("HttpHeaderConsumer::consume");
    
    if (m_firstConsume) {
        TRACE("firstConsume");
	TRACE(now);
        m_firstConsume = false;
	m_firstConsumeTime = now;
    }
    
    if (now - consumeStart() > getTimeout()) {
        TRACE("timeout!");
	TRACE(now);
	m_err = m_timedout = true;
	return false;
    } else if (!m_haveHeader) {
        Adafruit_WINC1500Client &client = m_ctxt.getClient();
        if (client.connected()) {
	    // if there are incoming bytes available
	    // from the host, read them and process them
	    int avail = client.available();
	    while (avail--) {
	        char c = client.read();
		assert(c, "Unexpected NULL char found in http response stream");
		m_response.add(c);
		if ((c == 0x0a) && m_gotCR && m_haveFirstCRLF) {
		    //DHL("CRLFCRLF");
		    // 2 CRLF marks end of HTTP response, with content to follow
		    m_haveHeader = true;
		    return false;
		} else if ((c == 0x0a) && m_gotCR) {
		    //DHL("CRLF");
		    m_gotCR = false;
		    m_haveFirstCRLF = true;
		} else if ((c == 0x0d) && m_haveFirstCRLF) {
		    //DHL("CRLFCR");
		    m_gotCR = true;
		} else {
		    m_gotCR = c == 0x0d;
		    m_haveFirstCRLF = false;
		}
	    }
	    return true;
	}
    }
    
    return false;
}


