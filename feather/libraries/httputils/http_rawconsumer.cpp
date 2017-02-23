#include <http_rawconsumer.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <MyWiFi.h>


bool HttpRawConsumer::consume(unsigned long now)
{
    if (HttpHeaderConsumer::consume(now)) {
        return true;
    } else {
        Adafruit_WINC1500Client &client = m_ctxt.getClient();
	if (client.connected()) {
	    DL("HttpRawConsumer::consume; consuming raw response");
	    int avail;
	    if (avail = client.available()) {
	        m_response.expand(m_response.len()+avail+1);
		uint8_t *s = (uint8_t*) m_response.c_str();
		int read = client.read(&s[m_response.len()], avail);
		if (read != avail) {
		    DL("HttpRawConsumer::consume; error while consuming raw response document");
		    return false;
		}
	    }
	    return true;
	} else {
	    DL("HttpRawConsumer::consume; done consuming raw response document");
	    return false;
	}
    }
}


