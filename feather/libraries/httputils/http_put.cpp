#include <http_put.h>

#include <wifiutils.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <http_headerconsumer.h>



/* STATIC */
const char *HttpPut::DefaultContentType = "application/x-www-form-urlencoded";


HttpPut::HttpPut(const char *ssid, const char *ssidPswd, 
		 const char *host, int port, const char *urlPieces[], 
		 const char *dbUser, const char *dbPswd, 
		 const char *contentType, bool isSSL)
  : HttpOp(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL),
    m_contentType(contentType), m_urlPieces(urlPieces)
{
}


void HttpPut::sendPUT(Stream &s) const
{
    s.print("PUT ");
    for (int i = 0; m_urlPieces[i]; i++)
      s.print(m_urlPieces[i]);
    s.print(" ");
    s.println(TAGHTTP11);
    sendHost(s);
    s.print("Content-Type: "); s.println(m_contentType.c_str());
    s.println("User-Agent: Arduino/1.0");
    s.println("Connection: close");
    s.print("Content-Length: ");
    s.println(getContentLength());
    s.println();
}


HttpResponseConsumer &HttpPut::getResponseConsumer()
{
    return getHeaderConsumer();
}


void HttpPut::sendDoc(Stream &s, const char *doc) const
{
    s.print(doc);
}


HttpPut::EventResult HttpPut::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpPut::event");
    TRACE("entry");
    
    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TRACE("ISSUE_OP");
        sendPUT(getContext().getClient());
	if (m_doc.len() > 0) {
	    sendDoc(getContext().getClient(), m_doc.c_str());
	}
	setOpState(ISSUE_OP_FLUSH);
	*callMeBackIn_ms = 10l;
	return CallMeBack;
    }
      break;
    default:
        return this->HttpOp::event(now, callMeBackIn_ms);
    }
}


void HttpPut::setDoc(const Str &docStr)
{
    m_doc = docStr;
}
