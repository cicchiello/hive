#include <http_couchconsumer.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>


static const char *DateTag = "Date: ";
static const char *ETag = "ETag: \"";


HttpCouchConsumer::HttpCouchConsumer(const WifiUtils::Context &ctxt)
  : HttpHeaderConsumer(ctxt)
{
    TF("HttpCouchConsumer::HttpCouchConsumer");
    init();
}


HttpCouchConsumer::~HttpCouchConsumer()
{
    DL("HttpCouchConsumer; DTOR");
}


void HttpCouchConsumer::init()
{
    m_content.clear();
}


void HttpCouchConsumer::reset()
{
    init();
    this->HttpHeaderConsumer::reset();
}


static inline char *advanceTo(char *start, char c)
{
    while (start && *start && (*start != c))
        start++;
    return start;
}


static inline bool checkSequence(const char *ptr, char c0, char c1, char c2, char c3, char c4, char c5)
{
    return (*ptr == c0) &&
      (*(ptr+1) == c1) &&
      (*(ptr+2) == c2) &&
      (*(ptr+3) == c3) &&
      (*(ptr+4) == c4) &&
      (*(ptr+5) == c5);
}


static inline bool checkSequence(const char *ptr, char c0, char c1, char c2, char c3)
{
    return (*ptr == c0) &&
      (*(ptr+1) == c1) &&
      (*(ptr+2) == c2) &&
      (*(ptr+3) == c3);
}


static inline char *advanceToSequence(char *start, char c0, char c1, char c2, char c3)
{
    while (start && *start && !checkSequence(start, c0, c1, c2, c3))
        start++;
    return start;
}


static inline const char *advanceToSequence(const char *start, char c0, char c1, char c2, char c3, char c4, char c5)
{
    while (start && *start && !checkSequence(start, c0, c1, c2, c3, c4, c5))
        start++;
    return start;
}


void HttpCouchConsumer::cleanChunkedResultInPlace(const char *terminationMarker)
{
    TF("HttpCouchConsumer::cleanChunkedResultInPlace");
    int i = 0;
    int len = getContent().len();
    const char *start = getContent().c_str();
    char *nonConstStart = (char*) start;
    char *ptr = nonConstStart;
    bool syntaxError = false;
    char *cr = ptr;
    syntaxError = (!cr || !*cr);
    if (!syntaxError) {
        TRACE("strting at first chunk size indicator");
	i += cr-ptr;
	ptr = cr;

	while (!syntaxError && (i < len)) {
	    cr = advanceTo(ptr, 0x0d);
	    if (!cr || !*cr || (cr-ptr < 1) || (cr-ptr > 4) || !*(cr+1) || (*(cr+1) != 0x0a)) {
	        TRACE("chunk parsing error");
		syntaxError = true;
	    }
				
	    if (!syntaxError) {
	        int chunkSz = StringUtils::ahextoi(ptr, cr-ptr);
		TRACE2("determined chunkSz: ", chunkSz);
		cr += 2;
		chunkSz += 2; // to account for CRLF at end of chunk
		TRACE2("taking chunkSz from here: ", cr);
		strncpy(ptr, cr, len - i - (cr-ptr));
		len -= cr-ptr;
		nonConstStart[len] = 0;
		i += chunkSz;
		ptr += chunkSz;
		
		TRACE2("remaining according to ptr: ", ptr);
		if (chunkSz == 0) {
		    // should be done
		    if ((cr != terminationMarker) || (i != len)) {
		        TRACE("chunk parsing error");
			syntaxError = true;
		    }
		}
	    }
	}
    }
    if (!syntaxError) {
        TRACE2("cleaned response: ", getContent().c_str());
	assert(i == len, "parsing programming error");
    }
}


void HttpCouchConsumer::parseHeaderLine(const StrBuf &line)
{
    TF("HttpCouchConsumer::parseHeaderLine");
 
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

bool HttpCouchConsumer::consume(unsigned long now)
{
    TF("HttpCouchConsumer::consume");

    if (HttpHeaderConsumer::consume(now)) {
        return true;
    } else {
        TRACE("consuming the couch part of the header response");
	Adafruit_WINC1500Client &client = m_ctxt.getClient();
	if (client.connected() && !isError() && hasOk()) {
	    TRACE("consuming couch response document; no error and hasOk");
	    
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
	    m_content.add(buf, i);
		
	    if (isChunked()) {
	        // check for chunked termination
	        const char *term = advanceToSequence(getContent().c_str(), 0x0a, '0',0x0d,0x0a,0x0d,0x0a);
		if (term && *term) {
		    TRACE("Found chunk termination");

		    // remove all the intra-chunk stuff from the buffer
		    cleanChunkedResultInPlace(term);

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


const char *HttpCouchConsumer::parseDoc(CouchUtils::Doc *doc) const
{
    DL("HttpCouchConsumer::parseDoc");
    return CouchUtils::parseDoc(m_content.c_str(), doc);
}
