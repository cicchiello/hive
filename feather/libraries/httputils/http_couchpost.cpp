#include <http_couchpost.h>


#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>


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


void HttpCouchPost::sendDoc(Stream &s, const CouchUtils::Doc &doc) const
{
    TF("HttpCouchPost::sendDoc");
#ifndef HEADLESS    
    CouchUtils::println(doc, Serial);
#endif
    CouchUtils::print(doc, s);
}


class NullStream : public Print {
private:
  int cnt;
public:
  NullStream() : cnt(0) {}
  
  int getLen() const {return cnt;}

  size_t write(uint8_t) {cnt++; return 1;}
};


HttpOp::EventResult HttpCouchPost::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpCouchPost::event");

    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TRACE("ISSUE_OP");
	
	NullStream nullStream;
	CouchUtils::print(m_content, nullStream);
	    
	sendPOST(getContext().getClient(), nullStream.getLen());
	if (nullStream.getLen() > 0) {
	    sendDoc(getContext().getClient(), m_content);
	}

	m_content.clear();
	    
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
