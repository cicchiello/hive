#include <http_couchpost.h>


#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>


HttpCouchPost::HttpCouchPost(const char *ssid, const char *ssidPswd, 
			     const char *host, int port, const char *page,
			     const CouchUtils::Doc &content,
			     const char *dbUser, const char *dbPswd, bool isSSL)
  : HttpCouchGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
    m_content(content)
{
}


HttpCouchPost::HttpCouchPost(const char *ssid, const char *ssidPswd, 
			     const IPAddress &hostip, int port, const char *page,
			     const CouchUtils::Doc &content,
			     const char *dbUser, const char *dbPswd, bool isSSL)
  : HttpCouchGet(ssid, ssidPswd, hostip, port, page, dbUser, dbPswd, isSSL),
    m_content(content)
{
}


void HttpCouchPost::sendPOST(Stream &s, int contentLength) const
{
    TF("HttpCouchPost::sendPOST");
    D("POST ");
    s.print("POST ");
    sendPage(s);
    D(" ");
    s.print(" ");
    DL(TAGHTTP11);
    s.println(TAGHTTP11);
    DL("User-Agent: Hive/0.0.1");
    s.println("User-Agent: Hive/0.0.1");
    sendHost(s);
    DL("Accept: */*");
    s.println("Accept: */*");
    D("Content-Type: ");
    s.print("Content-Type: ");
    DL("application/json");
    s.println("application/json");
    D("Content-Length: ");
    s.print("Content-Length: ");
    DL(contentLength);
    s.println(contentLength);
    DL("Connection: close");
    s.println("Connection: close");
    DL();
    s.println();
}


void HttpCouchPost::sendDoc(Stream &s, const char *doc) const
{
    TF("HttpCouchPost::sendDoc");
    PL(doc);
    s.print(doc);
}


HttpOp::EventResult HttpCouchPost::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpCouchPost::event");

    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TRACE("ISSUE_OP");
	
	Str str;
	CouchUtils::toString(m_content, &str);
	    
	sendPOST(getContext().getClient(), str.len());
	if (str.len() > 0) {
	    sendDoc(getContext().getClient(), str.c_str());
	}
	    
	setOpState(ISSUE_OP_FLUSH);
	
	*callMeBackIn_ms = 10l;
	return CallMeBack;
    }
      break;
    default:
        return this->HttpCouchGet::event(now, callMeBackIn_ms);
    }
}


bool HttpCouchPost::testSuccess() const
{
    DL("HttpCouchPost::testSuccess");
    return getHeaderConsumer().hasOk();
}
