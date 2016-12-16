#include <pulse.t.h>

#define NDEBUG
#include <strutils.h>

#include <Arduino.h>

#include <platformutils.h>


static bool success = true;

static unsigned long timeToAct = 500l;

bool PulseTest::setup() {
    pinMode(5, OUTPUT);

    PlatformUtils::nonConstSingleton().initPulseGenerator(0, 44100);

    unsigned long now = millis();
    timeToAct += now;

    return success;
}


static bool onOrOff = true;
static void Handler (void)
{
    digitalWrite(5, onOrOff ? HIGH : LOW);
    onOrOff = !onOrOff;
}


static bool started = false;
bool PulseTest::loop() {
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
	if (now >= timeToAct + 10000l) {
	    m_didIt = true;
	} else {
	    if (!started) {
	        PlatformUtils::nonConstSingleton().startPulseGenerator(0, Handler);
		started = true;
	    }
	}
    }

    return success;
}




