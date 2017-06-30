#include <http_headerconsumer.h>

#include <Arduino.h>

#define HEADLESS
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
const char *HttpHeaderConsumer::TAG404 = "HTTP/1.1 404 Object Not Found";



HttpHeaderConsumer::HttpHeaderConsumer(const WifiUtils::Context &ctxt)
  : HttpResponseConsumer(ctxt)
{
    TF("HttpHeaderConsumer::HttpHeaderConsumer");
    init();
}


void HttpHeaderConsumer::init()
{
    TF("HttpHeaderConsumer::init");
    m_haveHeader = m_gotCR = m_haveFirstCRLF = false;
    m_timedOut = m_err = m_hasOk = m_hasNotFound = m_isChunked = m_hasContentLength = false;
    m_contentLength = 0;
    m_firstConsume = true;
    m_firstConsumeTime = 0;
    m_line.clear();
}


void HttpHeaderConsumer::reset()
{
    init();
    this->HttpResponseConsumer::reset();
}


void HttpHeaderConsumer::parseHeaderLine(const StrBuf &line)
{
    TF("HttpHeaderConsumer::parseHeaderLine");
    if (!m_hasOk) {
        m_hasOk = (strstr(line.c_str(), TAG200) != NULL) ||
	          (strstr(line.c_str(), TAG201) != NULL);
    }
    if (!m_hasNotFound) {
        m_hasNotFound = (strstr(line.c_str(), TAG404) != NULL);
    }
    if (!m_hasContentLength && !m_isChunked) {
        const char *cl = strstr(line.c_str(), TAGContentLength);
	if (cl != NULL) {
	    cl = StringUtils::eatWhitespace(cl + strlen(TAGContentLength));
	    if (cl != NULL) {
	        TRACE("found (ok || notFound) && Content-Length");
		m_contentLength = atoi(cl);
		m_hasContentLength = true;
	    }
	} else {
	    const char *te = strstr(line.c_str(), TAGTransferEncoding);
	    if ((te != NULL) && strstr(te, TAGChunked)) {
	        TRACE("found ok and Chunked");
		m_isChunked = true;
	    }
	}
    }
}


#define BITESZ 40

bool HttpHeaderConsumer::consume(unsigned long now)
{
    TF("HttpHeaderConsumer::consume");
    if (m_firstConsume) {
        TRACE2("firstConsume; now : ", now);
        m_firstConsume = false;
	m_firstConsumeTime = now;
    }
    
    WiFiClient &client = m_ctxt.getClient();
    if (now - consumeStart() > getTimeout()) {
        TRACE2("timeout!  now: ", now);
	m_err = m_timedOut = true;
	client.stop();
	return false;
    } else if (!m_haveHeader) {
        if (client.connected()) {
	    char buf[BITESZ+2];
	    
	    // if there are incoming bytes available
	    // from the host, read them and process them

	    // but never process more than BITESZ chars to ensure the outter event loop
	    // isn't starved of time

	    int cnt = BITESZ, i = 0;
	    int avail = client.available();
	    if (cnt > avail) cnt = avail;
	    while (cnt--) {
	        char c = client.read();
		assert(c, "Unexpected NULL char found in http response stream");
		buf[i++] = c;
		buf[i] = 0;
		if ((c == 0x0a) && m_gotCR && m_haveFirstCRLF) {
		    //DHL("CRLFCRLF");
		    // 2 CRLF marks end of HTTP response, with content (optionally) to follow
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
		    m_haveFirstCRLF = false;
		    if (m_gotCR = c == 0x0d) {
			parseHeaderLine(m_line);
			m_line = "";
		    } else {
		        m_line.add(c);
		    }
		}
	    }
	    return true;
	} else {
	    PH("Not connected!?!?!");
	}
    }
    
    return false;
}

