#include <tests.h>

#define NDEBUG
#include <strutils.h>

#include <serial.t.h>
#include <couchutils.t.h>

#include <platformutils.h>

#include <Arduino.h>


static int s_numTests = 0;
static Test **s_testInstances = NULL;
static bool *s_testsInitialized = NULL;
static bool *s_testsDone = NULL;
static bool *s_testsPassed = NULL;

static bool err = false;
static char errbuf[80];

static void wdtEarlyWarningHandler()
{
    PF("wdtEarlyWarningHandler; ");
  
    // first, prevent the WDT from doing a full system reset by resetting the timer
    PlatformUtils::nonConstSingleton().clearWDT();

    PHL("BARK!");
    PL("");
    
    // Next, do a more useful system reset
    PlatformUtils::nonConstSingleton().resetToBootloader();
}


bool Test::isDone() const {return m_didIt;}

void Tests::init(int selectedTests[])
{
    // setup WDT
    PlatformUtils::nonConstSingleton().initWDT(wdtEarlyWarningHandler);

    s_numTests = 0;
    while (selectedTests[s_numTests] >= 0)
        ++s_numTests;
    
    s_testInstances = new Test*[s_numTests];
    s_testsInitialized = new bool[s_numTests];
    s_testsDone = new bool[s_numTests];
    s_testsPassed = new bool[s_numTests];
    
    for (int i = 0; i < s_numTests; i++) {
        Test *testInstance = NULL;
	switch (selectedTests[i]) {
	case SERIAL_TEST:
	    testInstance = new SerialTest();
	    break;
	case COUCHUTILS_TEST:
	    testInstance = new CouchUtilsTest();
	    break;
	default: {
	    err = true;
	    sprintf(errbuf, "Uknown test requested: %d", selectedTests[i]);
	}
	}
	s_testInstances[i] = testInstance;
	s_testsInitialized[i] = false;
	s_testsDone[i] = false;
	s_testsPassed[i] = true;
    }
}

Tests::Tests(int selectedTests[])
{
    init(selectedTests);
}


void Tests::setup()
{
    PF("Tests::setup; ");
    
    // initialize digital pin 13 as an output.
    pinMode(13, OUTPUT);
    
    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
	delay(25);
    }

    if (!err) {
        PH("Setting up tests; configured to run ");
	P(s_numTests);
	PL(" tests");
    } else {
        PH("There was an error encountered while configuring tests: ");
	PL(errbuf);
	s_numTests = 0;
    }

}


static int currTest = 0;
void Tests::loop()
{
    unsigned long now = millis();
    
    if (currTest < s_numTests) {
        if (!s_testsInitialized[currTest]) {
	    PL("");
	    PL("");
	    PL("");
	    P("Calling setup for test ");
	    PL(s_testInstances[currTest]->testName());
	    bool stat = s_testInstances[currTest]->setup();
	    s_testsInitialized[currTest] = true;
	    if (!stat) {
	        s_testsPassed[currTest] = false;
		s_testsDone[currTest] = true;
		PL("");
		PL("");
		currTest++;
	    }
	} else {
	    bool stat = s_testInstances[currTest]->loop();
	    if (!stat) {
	        s_testsPassed[currTest] = false;
		s_testsDone[currTest] = true;
		PL("");
		PL("");
		currTest++;
	    } else if (s_testInstances[currTest]->isDone()) {
	        s_testsPassed[currTest] = true;
		s_testsDone[currTest] = true;
		PL("");
		PL("");
		currTest++;
	    }
	}
    }

    if (currTest == s_numTests) {
	PL("\n");
	PL("\n");
        PL("Tests done! ");
	int passCnt = 0;
	for (int i = 0; i < s_numTests; i++) {
	    P(s_testInstances[i]->testName());
	    P(": ");
	    PL(s_testsPassed[i] ? "Passed" : "Failed");
	    passCnt += s_testsPassed[i] ? 1 : 0;
	}
	currTest++;

	PL("");
	PL("Destructing all tests");
	for (int i = 0; i < s_numTests; i++) {
	    delete s_testInstances[i];
	}
	
	PL("");
	PL("");
	P(passCnt);
	P(" of ");
	P(s_numTests);
	PL(" tests passed");
	
	PL("");
	PL("");
	P("SystemCoreClock: ");
	PL(SystemCoreClock);
	PL("");
	PL("Hit any key to return to the bootloader...");
    } else if (Serial.available() != 0) {
	PlatformUtils::nonConstSingleton().resetToBootloader();
    } else {
        PlatformUtils::nonConstSingleton().clearWDT();
    }
}



Tests::Tests()
{
    int selectedTests[] = {
	COUCHUTILS_TEST
#if 0
	,RTC_TEST
#endif
	,SDCARD_LS
        ,SDCARD_FNEXIST
	,SDCARD_READ
	,SDCARD_WRITE
	,SDCARD_RAWWRITE
	,SDCARD_FEXIST
	,SDCARD_RM
#if 0
	,ADC_SAMPLE
	,PULSE_TEST
	,SERIAL_TEST
	,WIFI_VERSION
	,WIFI_SCAN
	,WIFI_CONNECT
	,LISTEN_TEST
	,HTTP_GET_TEST
	,HTTP_PUT_TEST
	,HTTP_SSLGET_TEST
	,HTTP_SSLPUT_TEST
	,HTTP_BINARYGET_TEST
	,HTTP_BINARYPUT_TEST
	,WDT_TEST
	,BANK_TEST
	,PITCHER_TEST
#endif
//	,LIFI_TX_TEST
//	,LIFI_TEST
      ,-1
    };
    
    init(selectedTests);
}


