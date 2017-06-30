#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <SeqGetter.h>

#include <hiveconfig.h>
#include <hive_platform.h>

#include <Mutex.h>

#include <http_op.h>

#include <strbuf.h>


#define WIFISSID "JOE3"
#define WIFIPSWD "abcdef012345"
#define DBHOST "jfcenterprises.cloudant.com"
#define DBPORT "443"
#define ISSSL "true"
#define DBUSER "afteptsecumbehisomorther"
#define DBPSWD "e4f286be1eef534f1cddd6240ed0133b968b1c9a"
#define CHANNEL_DOCID "d359371b544d4d51202020470b0b01ff-app"

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

    HivePlatform::nonConstSingleton()->startWDT();
}



/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/


static Mutex sWifiMutex;
static SeqGetter *sSequenceGetter = NULL;
static Str sHiveChannelDocId(CHANNEL_DOCID);

void loop(void)
{
    TF("::loop");

    unsigned long now = millis();

    if (sSequenceGetter == NULL) {
        sSequenceGetter = new SeqGetter(sConfig, now, sHiveChannelDocId, &sWifiMutex);
    } else {
        if (sSequenceGetter->isItTimeYet(now)) {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    bool callBack = sSequenceGetter->loop(now);
	    if (!callBack) {
	      PH2("Done! now: ", millis());
	      if (sSequenceGetter->hasLastSeqId()) {
		  PH2("most recent sequence id: ", sSequenceGetter->getLastSeqId().c_str());
		  PH2("revision that corresponds to that seq id: ", sSequenceGetter->getRevision().c_str());
	      } else {
		  PH("Something went wrong!");
	      }
	      while (true) {
		  HivePlatform::nonConstSingleton()->clearWDT();
		  delay(1000);
	      };
	    }
	}
    }
}



