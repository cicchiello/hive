#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hiveconfig.h>
#include <hive_platform.h>

#include <sdutils.h>

#include <AppChannel.h>
#include <RateProvider.h>
#include <TimeProvider.h>
#include <Mutex.h>
#include <AudioUpload.h>

#include <http_op.h>

#include <str.h>
#include <strbuf.h>

#include <wifiutils.h>

#define WIFISSID "JOE3"
#define WIFIPSWD "abcdef012345"
#define DBHOST "jfcenterprises.cloudant.com"
#define DBPORT "443"
#define ISSSL "true"
#define DBUSER "afteptsecumbehisomorther"
#define DBPSWD "e4f286be1eef534f1cddd6240ed0133b968b1c9a"
    

static const char *ResetCause = "unknown";


class FrozenTimeProvider : public TimeProvider {
public:
  ~FrozenTimeProvider();
  void toString(unsigned long now, Str *str) const
  {
    TF("FrozenTimeProvider::toString");
    *str = "1491139135";
    PH2("testing time: ", str->c_str());
  }
  unsigned long getSecondsSinceEpoch(unsigned long now) const {return 1491139135l;}
};
FrozenTimeProvider::~FrozenTimeProvider() {}


static FrozenTimeProvider sFrozenTimeProvider;

const TimeProvider *GetTimeProvider()
{
    return &sFrozenTimeProvider;
}

static HiveConfig sConfig("unknown", "dev");

static HttpOp::YieldHandler sPrevYieldHandler = NULL;

void GlobalYield()
{
    if (sPrevYieldHandler != NULL)
        sPrevYieldHandler();
}

static Mutex sAppChannelWifiMutex, sSdMutex, sUploadWifiMutex;
static AppChannel *sAppChannel = NULL;



  
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

    pinMode(SDUtils::SPI_CS, OUTPUT);  // set the SPI_CS pin as output
    digitalWrite(SDUtils::SPI_CS, HIGH);

    CouchUtils::Doc doc(sConfig.getDoc());

    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::SsidProperty, CouchUtils::Item(WIFISSID)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::SsidPswdProperty, CouchUtils::Item(WIFIPSWD)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbHostProperty, CouchUtils::Item(DBHOST)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbPortProperty, CouchUtils::Item(DBPORT)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::IsSslProperty, CouchUtils::Item(ISSSL)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbUserProperty, CouchUtils::Item(DBUSER)));
    doc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbPswdProperty, CouchUtils::Item(DBPSWD)));
    
    sConfig.setDoc(doc);

    sAppChannel = new AppChannel(&sConfig, millis(), 0, &sAppChannelWifiMutex, &sSdMutex);

    HivePlatform::nonConstSingleton()->startWDT();
}



/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/

bool uploadLoop(unsigned long now);

static unsigned long sNextAppChannelAction = 0, sNextUploadAction = 0;
static bool sUploading = false, sFirstEntry = true;
void loop(void)
{
    TF("::loop");

    unsigned long now = millis();
    
    if (sFirstEntry) {
        sNextUploadAction = now + 7000l;
	sNextAppChannelAction = now;
        sFirstEntry = false;

	Str t;
	GetTimeProvider()->toString(now, &t);
	PH2("Time test: ", t.c_str());
    }

    
    HivePlatform::nonConstSingleton()->clearWDT();
    if ((now > sNextUploadAction) || (now > sNextAppChannelAction)) {
        if (now > sNextAppChannelAction) {
	    sNextAppChannelAction = millis() + 10l;
	    if (sAppChannel->loop(now)) {
	        if (sAppChannel->haveMessage()) {
		    StrBuf payload;
		    sAppChannel->getPayload(&payload);
		    PH2("Have command msg!  payload: ", payload.c_str());
		    sNextAppChannelAction = millis() + 2000l;
		}
	    }
	}
	
        if (now > sNextUploadAction) {
            if (!sUploading) {
	        PH2("Upload starting; now: ", now);
		sUploading = true;
	    }
	    bool callBack = uploadLoop(now);
	    if (!callBack) {
	        PH2("Upload done; now: ", now);
	        sNextUploadAction = millis() + 10000l;
		sUploading = false;
	    } else {
	        sNextUploadAction = millis();
	    }
	}
    }
}




class MyRateProvider : public RateProvider {
public:
  ~MyRateProvider();
  int secondsBetweenSamples() const {return 600;}
};
MyRateProvider::~MyRateProvider() {}
static MyRateProvider sRateProvider;

#define ATTACHMENT_CONTENT_TYPE  "audio/wav"
#define CONTENT_FILENAME         "/LISTEN.WAV"

static int sUploadState = 0;
static AudioUpload *sUploader;
static unsigned long sUploadStartTimestamp;
static int sUploadCnt = 0;
bool uploadLoop(unsigned long now)
{
    TF("::uploadLoop");

    bool callMeBack = true;
    switch (sUploadState) {
    case 0: {
        TF("::uploadLoop; case 0");
        PH2("uploading the existing audio clip; now:", millis());
	sUploadStartTimestamp = now;
	StrBuf attachmentDescription;
	attachmentDescription.append(20).append("s-audio-clip");
	StrBuf attachmentName;
	attachmentName.append("realtime-test").append(sUploadCnt++).append(".wav");
	sUploadState = 1;
	sUploader = new AudioUpload(sConfig, "uploader", attachmentDescription.c_str(),
				    attachmentName.c_str(), ATTACHMENT_CONTENT_TYPE,
				    CONTENT_FILENAME, sRateProvider, 
				    now, &sUploadWifiMutex, &sSdMutex);
    }; break;
      
    case 1: {
        TF("::uploadLoop; case 1");
        if (sUploader->isItTimeYet(now)) {
	    bool callMeBack = sUploader->loop(now);
	    if (!callMeBack) {
	        PH2("Audio Upload Success!  Total time: ", (millis()-sUploadStartTimestamp));
		delete sUploader;
		sUploader = NULL;
		sUploadState = 0;
		assert(sSdMutex.whoOwns() == NULL, "SD Mutex hasn't been released");
		assert(sUploadWifiMutex.whoOwns() == NULL, "Wifi Mutex hasn't been released");
		callMeBack = false;
	    }
	}
    }; break;
      
    default: PH2("Unexpected state: ", sUploadState);
    }

    return callMeBack;
}
