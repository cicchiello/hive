#include <Arduino.h>

//#define HEADLESS
//#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>
#include <str.h>
#include <strbuf.h>


#define CONNECTED_LED           10


void setup(void)
{
    const char *rcause = HivePlatform::singleton()->getResetCause();
  
    delay(500);

#ifndef HEADLESS
    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
#endif
  
    pinMode(CONNECTED_LED, OUTPUT);  // indicates BLE connection

    P("RCAUSE: "); PL(rcause);

    delay(500);

    PL("Mem.t console");
    PL("-------------------------");

    PL();

    DL("starting WDT");
    HivePlatform::nonConstSingleton()->startWDT();
    HivePlatform::singleton()->trace("wdt handler registered");
  
    PL();
}


bool memValidity()
{
    TF("::memValidity");
    const void *dummy = NULL;
    const void *stack = &dummy;
    const void *heap = new char[0];
    PH2("stack: ", ((long)stack));
    PH2("heap: ", ((long)heap));
    PH2("stack > heap ?  ", (((long)stack) > ((long)heap) ? "true" : "false"));
    return ((long)stack) > ((long)heap);
}


static const char *testString = "Now is the time for all good men to come to the aid of their country";
static const char *reallyLongString = "Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country, Now is the time for all good men to come to the aid of their country";

void heapTest()
{
    TF("::heapText");

    int size = 2;
    const void *base = new char[0];
    while (size < strlen(reallyLongString)) {
        Str t;
	{
	  StrBuf tt;
	  tt.expand(size+1);
	  const char *ptr = tt.c_str();
	  char *nonConstPtr = (char*) ptr;
	  strncpy(nonConstPtr, reallyLongString, size);
	  nonConstPtr[size] = 0;
	  t = ptr;
	}
	  
	PH2("size: ", size);
	PH2("base: ", (long)(base));
	PH2("t.buf: ", (long)((void*)t.c_str()));
	long delta = ((long)((void*)t.c_str())) - ((long)base);
	PH2("delta: ", delta);

	base = t.c_str();
	size *= 2;
	memValidity();
    }
}


void stackTest(int depth, const char *str)
{
    TF("::stackTest");
    const void *t1 = NULL;
    char buf[1024];
    const void *t2 = NULL;

    PH2("depth: ", depth);
    PH2("t1: ", (long)((void*) &t1));
    PH2("t2: ", (long)((void*) &t2));
    PH("");
    for (int i = 0; i < 1024; i++)
      buf[i] = 0x5a;

    memValidity();
    if (depth > 0) {
      stackTest(depth-1, buf);
    }

    heapTest();
}


static int state = 0;
void loop(void)
{
    TF("::loop");
    
    unsigned long now = millis();
    HivePlatform::nonConstSingleton()->clearWDT();

    switch (state) {
    case 0: 
      if (now > 5000) {
        stackTest(10, 0);
	state++;
      }
      break;
    case 1:
      if (now > 10000) {
	heapTest();
	state++;
      }
      break;
    case 2:
      break;
    }
}

