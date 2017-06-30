#include <Arduino.h>

//#define HEADLESS
//#define NDEBUG

#include <Trace.h>

#include <SeqListener.h>

#include <hiveconfig.h>
#include <hive_platform.h>

#include <http_op.h>

#include <strbuf.h>


#define WIFISSID "JOE3"
#define WIFIPSWD "abcdef012345"
#define DBHOST "jfcenterprises.cloudant.com"
#define DBPORT "443"
#define ISSSL "true"
#define DBUSER "afteptsecumbehisomorther"
#define DBPSWD "e4f286be1eef534f1cddd6240ed0133b968b1c9a"
#define HIVEID "d359371b544d4d51202020470b0b01ff-app"

#define MOST_RECENT_MSGID "d8a4bd93f294c5dd76db221314f9d9a4"

static const char *ResetCause = "unknown";


static HiveConfig sConfig("unknown", "dev");

static HttpOp::YieldHandler sPrevYieldHandler = NULL;

extern "C" void GlobalYield()
{
    if (sPrevYieldHandler != NULL)
        sPrevYieldHandler();
}


  
/**************************************************************************/
/*!
    @brief  Sets up the HW (this function is called automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
    TF("::setup");
    
    // capture the reason for previous reset 
    ResetCause = HivePlatform::singleton()->getResetCause(); 
    
    delay(500);

#ifndef HEADLESS
    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
#endif

    PH2("ResetCause: ", ResetCause);

    //pinMode(5, OUTPUT);         // for debugging: used to indicate ISR invocations for motor drivers
    
    delay(500);
    
    PL("---------------------------------------");

    PH("Setup done...");

    CouchUtils::Doc doc(sConfig.getDoc());

    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::SsidProperty, CouchUtils::Item(WIFISSID)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::SsidPswdProperty, CouchUtils::Item(WIFIPSWD)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbHostProperty, CouchUtils::Item(DBHOST)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbPortProperty, CouchUtils::Item(DBPORT)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::IsSslProperty, CouchUtils::Item(ISSSL)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbUserProperty, CouchUtils::Item(DBUSER)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbPswdProperty, CouchUtils::Item(DBPSWD)));
    
    sConfig.setDoc(doc);

    sConfig.addProperty(SeqListener::APP_CHANNEL_MSGID_PROPNAME, MOST_RECENT_MSGID);
    
    HivePlatform::nonConstSingleton()->startWDT();
    delay(1000);    
}



/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/


static SeqListener *sSequenceListener = NULL;
static Str sHiveId(HIVEID);

static bool sIsOnline = false;

void loop(void)
{
    TF("::loop");

    unsigned long now = millis();
    if (sSequenceListener == NULL) {
        sSequenceListener = new SeqListener(&sConfig, now, sHiveId);
    } else {
        bool newIsOnline = sSequenceListener->isOnline();
	if (newIsOnline && !sIsOnline) {
	    PH2("detected online; now: ", now);
	    sIsOnline = true;
	    TRACE2("LastRevision: ", sSequenceListener->getLastRevision().c_str()); 
	} else if (!newIsOnline && sIsOnline) {
	    PH2("detected offline; now: ", now);
	    sIsOnline = false;
	}
	
        if (sSequenceListener->isItTimeYet(now)) {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    sSequenceListener->loop(now);
            if (sSequenceListener->hasPayload()) {
	        PH("......");
	        PH2("HAVE_PAYLOAD; now: ", now);
		PH2("MsgId: ", sSequenceListener->getMsgId().c_str()); 
		PH2("Revision: ", sSequenceListener->getRevision().c_str()); 
		PH2("PrevMsgId: ", sSequenceListener->getPrevMsgId().c_str()); 
		PH2("Payload: ", sSequenceListener->getPayload().c_str()); 
		PH2("Timestamp: ", sSequenceListener->getTimestamp().c_str()); 
	        PH("......");

		PH("Simulating time to act on payload (10s)...");
		for (int i = 0; i < 10; i++) {
		    HivePlatform::nonConstSingleton()->clearWDT();
		    delay(1000);
		}
	        PH("......");
		
	        sSequenceListener->restartListening();
	    }
	}
    }
}



