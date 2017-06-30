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
		StrBuf something;
		something.expand(avail+1);
		uint8_t *s = (uint8_t*) something.c_str();
		int read = client.read(s, avail);
		if (read != avail) {
		    DL("HttpRawConsumer::consume; error while consuming raw response document");
		    return false;
		}
		s[avail] = 0;
		m_content.add(something.c_str(), avail);
	    }
	    return true;
	} else {
	    DL("HttpRawConsumer::consume; done consuming raw response document");
	    return false;
	}
    }
}


