#include <bank.t.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <ota.h>

#include <Arduino.h>


static bool success = true;

extern int main();

bool BankTest::setup()
{
    TF("BankTest::setup");
    
    PH("Initializing BankTest");

    long entry_point = (long) &main;
    PH("Sketch entry point: "); PL(entry_point);
    
    switch (OTA::singleton().getThisFlashBank()) {
    case OTA::Bank0:
      success = (0x2000 <= entry_point) && (entry_point < 0x20000);
      if (!success) PH("BankTest failed in Bank0 setup case");
      break;
    case OTA::Bank1:
      success = (0x22000 <= entry_point) && (entry_point < 0x40000);
      if (!success) PH("BankTest failed in Bank1 setup case");
      break;
    default:
      success = false;
      if (!success) PH("BankTest failed in default setup case");
    }
    
    PH("end of setup; success = "); PL(success);
    return success;
}


enum OTA_TESTS {
  Start,
  DownloadImage,
  Erase,
  Done
};
static OTA_TESTS s_state = Start;


bool BankTest::loop() {
    TF("BankTest::loop");
    
    switch (s_state) {
    case Start:
        s_state = DownloadImage;
	break;
    case DownloadImage: {
        s_state = Erase;
    };
      break;
    case Erase: {
        PH("Erase test");
        unsigned char *dst = NULL;
	uint32_t len = 0;
        if (OTA::singleton().getThisFlashBank() == OTA::Bank0) {
	    dst = (unsigned char*) 0x22000;
	    len = 0x40000-0x22000;
	} else {
	    dst = (unsigned char*) 0x2000;
	    len = 0x20000-0x2000;
	}
	OTA::nonConstSingleton().eraseRange(dst, len);
	for (int i = 0; (i < len) && success; i++)
	    success = *(dst + i) == 0xff;
        if (!success) 
	    m_didIt = true;
	s_state = Done;
    };
      break;
    case Done:
        m_didIt = true;
    }
    
    return success;
}
