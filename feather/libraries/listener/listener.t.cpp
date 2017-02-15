#include <listener.t.h>

//#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <listener.h>

// adcdma
//  analog A5


#define ADCPIN A2
#define HWORDS 1024


static bool success = true;

static unsigned long timeToAct = 1000l;
static Listener *listener = NULL;
static bool captureDone = false;

bool ListenTest::setup()
{
    timeToAct += millis();

    return success;
}

bool ListenTest::loop() {
    TF("ListenTest::loop");
    const bool verbose = false;
    if ((millis() > timeToAct) && !m_didIt) {
        if (listener == NULL) {
	    const char *filename = "LISTEN.WAV";

	    listener = new Listener(ADCPIN, A0);
	    TRACE2("Listening; time (ms) : ", millis());
	    bool stat = listener->record(10500, filename, verbose);
	    assert(stat, "Couldn't start recording");
	} else {
	    success = listener->loop(verbose);
	    if (listener->isDone()) {
	        if (listener->hasError()) {
		    TRACE2("Failed: ", listener->getErrmsg());
		} else {
		    TRACE("Done: success! "); 
		}
		m_didIt = true;
		delete listener;
		listener = NULL;
	    }
	}
    }

    return success;
}


