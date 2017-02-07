#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>
#include <str.h>


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

    PL("Blinky console");
    PL("-------------------------");

    PL();

    DL("starting WDT");
    HivePlatform::nonConstSingleton()->startWDT();
    HivePlatform::singleton()->trace("wdt handler registered");
  
    PL();
}


#define INIT 0
#define SLOW_BLINK1             (INIT+1)
#define LOOP                    SLOW_BLINK1
#define SLOW_BLINK2             (SLOW_BLINK1+1)
#define SLOW_BLINK3             (SLOW_BLINK2+1)
#define FAST_BLINK1             (SLOW_BLINK3+1)
#define FAST_BLINK2             (FAST_BLINK1+1)
#define FAST_BLINK3             (FAST_BLINK2+1)
#define OFF_STATE               (FAST_BLINK3+1)
#define PAUSE_STATE             (OFF_STATE+1)

#define FAST_BLINK_TIME         300
#define SLOW_BLINK_TIME         900
#define OFF_TIME                200

static int s_mainState = INIT;
static int s_nextOnState = INIT;
static int s_lastTransitionTime = 0;

void loop(void)
{
    TF("::loop");
    
    unsigned long now = millis();
    HivePlatform::nonConstSingleton()->clearWDT();
    switch (s_mainState) {
    case INIT: {
        TRACE2("INIT ", INIT);
	digitalWrite(CONNECTED_LED, HIGH);
	
	s_lastTransitionTime = now;
	s_mainState = LOOP;

	TRACE("Everything initialized");
    }
      break;

    case SLOW_BLINK1:
      if (now > s_lastTransitionTime+SLOW_BLINK_TIME) {
	TRACE2("SLOW_BLINK1 ",SLOW_BLINK1);
	
	digitalWrite(CONNECTED_LED, LOW);

	s_lastTransitionTime = now;
	s_mainState = OFF_STATE;
	s_nextOnState = SLOW_BLINK2;
      }
      break;
      
    case SLOW_BLINK2:
      if (now > s_lastTransitionTime+SLOW_BLINK_TIME) {
	TRACE2("SLOW_BLINK2 ", SLOW_BLINK2);
	
	digitalWrite(CONNECTED_LED, LOW);

	s_lastTransitionTime = now;
	s_mainState = OFF_STATE;
	s_nextOnState = SLOW_BLINK3;
      }
      break;
      
    case SLOW_BLINK3:
      if (now > s_lastTransitionTime+SLOW_BLINK_TIME) {
	TRACE2("SLOW_BLINK3 ", SLOW_BLINK3);
	
	digitalWrite(CONNECTED_LED, LOW);

	s_lastTransitionTime = now;
	s_mainState = PAUSE_STATE;
	s_nextOnState = FAST_BLINK1;
      }
      break;
      
    case FAST_BLINK1:
      if (now > s_lastTransitionTime+FAST_BLINK_TIME) {
	TRACE2("FAST_BLINK1 ", FAST_BLINK1);
	
	digitalWrite(CONNECTED_LED, LOW);

	s_lastTransitionTime = now;
	s_mainState = OFF_STATE;
	s_nextOnState = FAST_BLINK2;
      }
      break;
      
    case FAST_BLINK2:
      if (now > s_lastTransitionTime+FAST_BLINK_TIME) {
	TRACE2("FAST_BLINK2 ", FAST_BLINK2);
	
	digitalWrite(CONNECTED_LED, LOW);

	s_lastTransitionTime = now;
	s_mainState = OFF_STATE;
	s_nextOnState = FAST_BLINK3;
      }
      break;
      
    case FAST_BLINK3:
      if (now > s_lastTransitionTime+FAST_BLINK_TIME) {
	TRACE2("FAST_BLINK3 ", FAST_BLINK3);
	
	digitalWrite(CONNECTED_LED, LOW);

	s_lastTransitionTime = now;
	s_mainState = PAUSE_STATE;
	s_nextOnState = SLOW_BLINK1;
      }
      break;
      
    case PAUSE_STATE:
      if (now > s_lastTransitionTime+(6*OFF_TIME)) {
	TRACE2("PAUSE_STATE ", PAUSE_STATE);
	
	digitalWrite(CONNECTED_LED, HIGH);

	s_lastTransitionTime = now;
	s_mainState = s_nextOnState;
      }
      break;
      
    case OFF_STATE:
      if (now > s_lastTransitionTime+OFF_TIME) {
	TRACE2("OFF_STATE ", OFF_STATE);
	
	digitalWrite(CONNECTED_LED, HIGH);

	s_lastTransitionTime = now;
	s_mainState = s_nextOnState;
	TRACE2("Transitioning to: ", s_nextOnState);
      }
      break;
      
    default:
      HivePlatform::singleton()->error("Unknown s_mainState in loop; quitting");
    };
}

