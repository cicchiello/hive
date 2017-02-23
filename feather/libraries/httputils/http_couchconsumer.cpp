#include <http_couchconsumer.h>

#define HEADLESS
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
}


void HttpCouchConsumer::reset()
{
    init();
    this->HttpHeaderConsumer::reset();
}


static inline const char *advanceTo(const char *start, char c)
{
    while (start && *start && (*start != c))
        start++;
    return start;
}


static inline bool checkSequence(const char *ptr, char c0, char c1, char c2, char c3, char c4)
{
    return (*ptr == c0) &&
      *(ptr+1) && (*(ptr+1) == c1) &&
      *(ptr+2) && (*(ptr+2) == c2) &&
      *(ptr+3) && (*(ptr+3) == c3) &&
      *(ptr+4) && (*(ptr+4) == c4);
}


static inline bool checkSequence(const char *ptr, char c0, char c1, char c2, char c3)
{
    return (*ptr == c0) &&
      *(ptr+1) && (*(ptr+1) == c1) &&
      *(ptr+2) && (*(ptr+2) == c2) &&
      *(ptr+3) && (*(ptr+3) == c3);
}


static inline const char *advanceToSequence(const char *start, char c0, char c1, char c2, char c3)
{
    while (start && *start && !checkSequence(start, c0, c1, c2, c3))
        start++;
    return start;
}


static inline const char *advanceToSequence(const char *start, char c0, char c1, char c2, char c3, char c4)
{
    while (start && *start && !checkSequence(start, c0, c1, c2, c3, c4))
        start++;
    return start;
}


void HttpCouchConsumer::cleanChunkedResult(const char *terminationMarker)
{
    TF("HttpCouchConsumer::cleanChunkedResult");
    Str cleaned;
    int i = 0;
    const char *ptr = m_response.c_str();
    bool syntaxError = false;
    const char *cr = advanceToSequence(ptr, 0x0d, 0x0a, 0x0d, 0x0a);
    syntaxError = (!cr || !*cr);
    if (!syntaxError) {
        TRACE("Advanced to end of first chunk");
	// found CRLFCRLF -- next should be a chunk size
	cr += 4;
				    
	// but first, transfer everything up to here
	for (const char *t = ptr; t < cr; i++)
	    cleaned.add(*t++);
	ptr = cr;
	
	TRACE2("cleaned so far: ", cleaned.c_str());
	while (!syntaxError && (i < m_response.len())) {
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
		i += cr-ptr;
		for (const char *t = cr; t < cr+chunkSz; i++)
		    cleaned.add(*t++);
		TRACE2("cleaned so far: ", cleaned.c_str());
		
		ptr = cr + chunkSz;
		TRACE2("remaining according to ptr: ", ptr);
		if (chunkSz == 0) {
		    // should be done
		    if ((cr != terminationMarker) || (i != m_response.len())) {
		        TRACE("chunk parsing error");
			syntaxError = true;
		    }
		}
	    }
	}
    }
    if (!syntaxError) {
        TRACE2("Cleaned response: ", cleaned.c_str());
	TRACE2("i: ", i);
	assert(i == m_response.len(), "parsing programming error");
	m_response = cleaned;
    }
}


bool HttpCouchConsumer::consume(unsigned long now)
{
    TF("HttpCouchConsumer::consume");
    if (HttpHeaderConsumer::consume(now)) {
        return true;
    } else {
        TRACE("consuming the couch part of the header response");
	Adafruit_WINC1500Client &client = m_ctxt.getClient();
	if (client.connected() && !isError() && hasOk()) {
	    TRACE("consuming couch response document");
	    int avail = client.available();
	    TRACE("no error and hasOk");

	    while (avail--) {
	        char c = client.read();
		m_response.add(c);
	    }
		
	    if (isChunked()) {
	        // check for chunked termination
	        const char *term = advanceToSequence(m_response.c_str(), '0',0x0d,0x0a,0x0d,0x0a);
		if (term && *term) {
		    TRACE("Found chunk termination");

		    // remove all the intra-chunk stuff from the buffer
		    cleanChunkedResult(term);
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
    return CouchUtils::parseDoc(getResponse().c_str(), doc);
}
