#include <http_couchput.h>


//#define HEADLESS
//#define NDEBUG
#include <strutils.h>

#include <Trace.h>


HttpCouchPut::HttpCouchPut(const char *ssid, const char *ssidPswd, 
			   const char *host, int port, const char *page,
			   const CouchUtils::Doc &content,
			   const char *credentials, bool isSSL)
  : HttpCouchGet(ssid, ssidPswd, host, port, page, credentials, isSSL),
    m_content(content)
{
}


HttpCouchPut::HttpCouchPut(const char *ssid, const char *ssidPswd, 
			   const IPAddress &hostip, int port, const char *page,
			   const CouchUtils::Doc &content,
			   const char *credentials, bool isSSL)
  : HttpCouchGet(ssid, ssidPswd, hostip, port, page, credentials, isSSL),
    m_content(content)
{
}


void HttpCouchPut::sendPUT(Stream &s, int contentLength) const
{
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
    PL("application/json");
    s.println("application/json");
    P("Content-Length: ");
    s.print("Content-Length: ");
    PL(contentLength);
    s.println(contentLength);
    PL("Connection: close");
    s.println("Connection: close");
    PL();
    s.println();
}


void HttpCouchPut::sendDoc(Stream &s, const char *doc) const
{
    TF("HttpCouchPut::sendDoc");
    PL(doc);
    s.print(doc);
}


HttpOp::EventResult HttpCouchPut::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpCouchPut::event");
    TRACE("entry");

    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TRACE("ISSUE_OP");
        WiFiClient &client = getContext().getClient();
	
	Str str;
	CouchUtils::toString(m_content, &str);
	    
	Str encodedStr;
	CouchUtils::urlEncode(str.c_str(), &encodedStr);

	sendPUT(client, encodedStr.len());
	if (str.len() > 0) {
	    sendDoc(client, encodedStr.c_str());
	    client.flush();
	}
	    
	setOpState(CONSUME_RESPONSE);
	*callMeBackIn_ms = 10l;
	return CallMeBack;
    }
      break;
    default:
        return this->HttpCouchGet::event(now, callMeBackIn_ms);
    }
}


bool HttpCouchPut::testSuccess() const
{
    DL("HttpCouchPut::testSuccess");
    return getHeaderConsumer().hasOk();
}
