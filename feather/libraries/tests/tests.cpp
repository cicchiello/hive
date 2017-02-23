#include <tests.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <sdcard_ls.t.h>
#include <sdcard_write.t.h>
#include <sdcard_read.t.h>
#include <sdcard_rawwrite.t.h>
#include <sdcard_rm.t.h>
#include <sdcard_fexist.t.h>
#include <sdcard_fnexist.t.h>
#include <wifi_version.t.h>
#include <wifi_scan.t.h>
#include <wifi_connect.t.h>
#include <dac_sin.t.h>
#include <adc_sample.t.h>
#include <listener.t.h>
#include <serial.t.h>
#include <couchutils.t.h>
#include <http_get.t.h>
#include <http_put.t.h>
#include <http_sslget.t.h>
#include <http_sslput.t.h>
#include <http_binput.t.h>
#include <http_fileput.t.h>
#include <http_sslfileput.t.h>
#include <http_sslbinput.t.h>
#include <wdt.t.h>
//#include <pitcher.t.h>
//#include <catcher.t.h>
#include <rtc.t.h>

#include <platformutils.h>

#include <Arduino.h>


/* STATIC */
char Test::ssid[] = "JOE5";     //  your network SSID (name)
char Test::pass[] = "abcdef012345";  // your network password


static int s_numTests = 0;
static Test **s_testInstances = NULL;
static bool *s_testsInitialized = NULL;
static bool *s_testsDone = NULL;
static bool *s_testsPassed = NULL;

static bool err = false;
static char errbuf[80];

static const char *sTestNames[] = {
      "RTC_TEST",
      "SDCARD_FNEXIST",
      "SDCARD_LS",
      "SDCARD_WRITE",
      "SDCARD_RAWWRITE",
      "SDCARD_FEXIST",
      "SDCARD_READ",
      "SDCARD_RM",
      "WIFI_VERSION",
      "WIFI_SCAN",
      "WIFI_CONNECT",
      "SERIAL_TEST",
      "COUCHUTILS_TEST",
      "HTTP_GET_TEST",
      "HTTP_SSLGET_TEST",
      "HTTP_PUT_TEST",
      "HTTP_SSLPUT_TEST",
      "HTTP_BINARYPUT_TEST",
      "HTTP_FILEPUT_TEST",
      "HTTP_SSLFILEPUT_TEST",
      "HTTP_SSLBINARYPUT_TEST",
      "DAC_SIN_TEST",
      "ADC_SAMPLE",
      "LISTEN_TEST",
      "WDT_TEST",
//      "PITCHER_TEST",
//      "CATCHER_TEST"
      NULL
    };

static void wdtEarlyWarningHandler()
{
    // first, prevent the WDT from doing a full system reset by resetting the timer
    PlatformUtils::nonConstSingleton().clearWDT();

    TF("wdtEarlyWarningHandler; BARK!");
    PH("");

    if (PlatformUtils::s_traceStr != NULL) {
        P("WDT Trace message: ");
	PL(PlatformUtils::s_traceStr);
    } else {
        PL("No WDT trace message registered");
    }
    
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
    
    for (int i = 0; !err && i < s_numTests; i++) {
        Test *testInstance = NULL;
	switch (selectedTests[i]) {
	case RTC_TEST:
	    testInstance = new RTCTest();
	    break;
	case SDCARD_FNEXIST:
	    testInstance = new SDCardFNExist();
	    break;
	case SDCARD_LS:
	    testInstance = new SDCardLS();
	    break;
	case SDCARD_WRITE:
	    testInstance = new SDCardWrite();
	    break;
	case SDCARD_RAWWRITE:
	    testInstance = new SDCardRawWrite();
	    break;
	case SDCARD_FEXIST:
	    testInstance = new SDCardFExist();
	    break;
	case SDCARD_READ:
	    testInstance = new SDCardRead();
	    break;
	case SDCARD_RM:
	    testInstance = new SDCardRm();
	    break;
	case WIFI_VERSION:
	    testInstance = new WifiVersion();
	    break;
	case WIFI_SCAN:
	    testInstance = new WifiScan();
	    break;
	case WIFI_CONNECT:
	    testInstance = new WifiConnect();
	    break;
	case SERIAL_TEST:
	    testInstance = new SerialTest();
	    break;
	case COUCHUTILS_TEST:
	    testInstance = new CouchUtilsTest();
	    break;
	case HTTP_GET_TEST:
	    testInstance = new HttpGetTest();
	    break;
	case HTTP_SSLGET_TEST:
	    testInstance = new HttpSSLGetTest();
	    break;
	case HTTP_PUT_TEST:
	    testInstance = new HttpPutTest();
	    break;
	case HTTP_SSLPUT_TEST:
	    testInstance = new HttpSSLPutTest();
	    break;
	case HTTP_BINARYPUT_TEST:
	    testInstance = new HttpBinaryPutTest();
	    break;
	case HTTP_FILEPUT_TEST:
	    testInstance = new HttpFilePutTest();
	    break;
	case HTTP_SSLFILEPUT_TEST:
	    testInstance = new HttpSSLFilePutTest();
	    break;
	case HTTP_SSLBINARYPUT_TEST:
	    testInstance = new HttpSSLBinaryPutTest();
	    break;
	case DAC_SIN_TEST:
	    testInstance = new DAC_SinTest();
	    break;
	case ADC_SAMPLE:
	    testInstance = new ADC_Sample();
	    break;
	case LISTEN_TEST:
	    testInstance = new ListenTest();
	    break;
	case WDT_TEST:
	    testInstance = new WDTTest();
	    break;
//	case PITCHER_TEST:
//	    testInstance = new PitcherTest();
//	    break;
//	case CATCHER_TEST:
//	    testInstance = new CatcherTest();
//	    break;
	default: {
	    err = true;
	    sprintf(errbuf,
		    "Unknown test requested: %d %s",
		    selectedTests[i], sTestNames[selectedTests[i]]);
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
    TF("Tests::setup");
    
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
static Str sCurrTestName;
void Tests::loop()
{
    TF("Tests::loop");
    
    unsigned long now = millis();
    
    if (currTest < s_numTests) {
        PlatformUtils::nonConstSingleton().clearWDT();
        if (!s_testsInitialized[currTest]) {
	    sCurrTestName = s_testInstances[currTest]->testName();
	    PL("");
	    PL("");
	    PL("");
	    P("Calling setup for test ");
	    PL(s_testInstances[currTest]->testName());
	    WDT_TRACE(sCurrTestName.c_str());
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
//      RTC_TEST,
#if 0      
      SDCARD_FNEXIST,
      SDCARD_LS,
      SDCARD_WRITE,
      SDCARD_RAWWRITE,
      SDCARD_FEXIST,
      SDCARD_READ,
      SDCARD_RM,
      WIFI_VERSION,
      WIFI_SCAN,
      WIFI_CONNECT,
      SERIAL_TEST,
#endif      
      COUCHUTILS_TEST,
#if 0
      HTTP_GET_TEST,
      HTTP_SSLGET_TEST,
      HTTP_PUT_TEST,
      HTTP_SSLPUT_TEST,
      HTTP_BINARYPUT_TEST,
      HTTP_FILEPUT_TEST,
      HTTP_SSLFILEPUT_TEST,
//      HTTP_SSLBINARYPUT_TEST,
      DAC_SIN_TEST,
      ADC_SAMPLE,
      LISTEN_TEST,
      WDT_TEST,
//      PITCHER_TEST,
//      CATCHER_TEST,
#endif
      -1
    };
    
    init(selectedTests);
}


