#include <http_jsonconsumer.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>


static const char *DateTag = "Date: ";
static const char *ETag = "ETag: \"";

HttpJSONConsumer::HttpJSONConsumer(const WifiUtils::Context &ctxt)
  : HttpHeaderConsumer(ctxt), mConsumer(&mDoc), mParser(&mConsumer)
{
    TF("HttpJSONConsumer::HttpJSONConsumer");
    init();
}


HttpJSONConsumer::~HttpJSONConsumer()
{
    TF("HttpJSONConsumer; DTOR");
}


void HttpJSONConsumer::init()
{
    mConsumer.clear();
    mParser.clear();
}


void HttpJSONConsumer::reset()
{
    init();
    this->HttpHeaderConsumer::reset();
}


void HttpJSONConsumer::parseHeaderLine(const StrBuf &line)
{
    TF("HttpJSONConsumer::parseHeaderLine");

    if (mEtag.len() == 0) {
        const char *ETagStr = strstr(line.c_str(), ETag);
	if (ETagStr != NULL) {
	    mEtag = "";
	    ETagStr += strlen(ETag);
	    const char *start = ETagStr;
	    while (ETagStr && *ETagStr && (*ETagStr != '"'))
	        ETagStr++;
	    mEtag.add(start, ETagStr-start);
	    TRACE2("Received ETag: ", mEtag.c_str());
	}
    }
    if (mTimestamp.len() == 0) {
        const char *dateStr = strstr(line.c_str(), DateTag);
	if (dateStr != NULL) {
	    dateStr += strlen(DateTag);
            while ((*dateStr != 0x0d) && (*dateStr != 0x0a) && *dateStr) {
	        mTimestamp.add(*dateStr++);
	    }
	    PH2("Received timestamp: ", mTimestamp.c_str());
	}
    }
    HttpHeaderConsumer::parseHeaderLine(line);
}


#define BITESZ 40

bool HttpJSONConsumer::consume(unsigned long now)
{
    TF("HttpJSONConsumer::consume");

    if (HttpHeaderConsumer::consume(now)) {
        return true;
    } else {
        TRACE("consuming the couch part of the header response");
	Adafruit_WINC1500Client &client = m_ctxt.getClient();
	if (client.connected() && !isError() && hasOk()) {
	    TRACE("consuming couch response document; no error and hasOk");
	    TRACE2("isChunked: ", (isChunked() ? "true" : "false"));
	    mParser.setIsChunked(isChunked());
	    
	    char buf[BITESZ+2]; 
	    
	    // if there are incoming bytes available
	    // from the host, read them and process them

	    // but never process more than BITESZ chars to ensure the outter event loop
	    // isn't starved of time

	    int cnt = BITESZ, i = 0;
	    int avail = client.available();
	    if (cnt > avail) cnt = avail;

	    while (cnt--) {
	        buf[i++] = client.read();
	    }
	    buf[i] = 0;
	    mParser.streamParseDoc(buf);

	    if (isChunked()) {
	        // check for termination
	        if (mConsumer.haveDoc()) {
		    TRACE("Have a complete response");
		    
		    client.stop();
		    return false; // indicate done consuming
		}
	    }
	} else {
	    TRACE("done consuming");
	    assert(isError() || hasOk() || hasNotFound(), "isError() || hasOk() || hasNotFound()");
	    client.stop();
	    return false; // indicate done consuming
	}
	return true; // indicate continue consuming
    }
}

