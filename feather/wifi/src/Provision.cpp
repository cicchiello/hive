#include <Provision.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <wifiutils.h>

#include <hiveconfig.h>
#include <ConfigReader.h>
#include <ConfigWriter.h>
#include <couchutils.h>
#include <Mutex.h>
#include <hive_platform.h>

#include <strutils.h>

#include <MyWiFi.h>
#include <WiFiClient.h>


#define NOOP              0
#define START_AP          1
#define LOAD_CONFIG       2
#define STOP_AP           3
#define DONE              7

#define INIT              1
#define LOADING           3
#define START_WEB_SERVICE 5
#define WEB_LISTEN        6
#define WEB_CLIENT        7
#define DELAY             8
#define SHUTDOWN          9

#ifndef NULL
#define NULL 0
#endif


class ProvisionImp {
  public:
    unsigned long mNextActionTime;
    int mMajorState, mMinorState, mDelayCnt, mPostDelayMinorState;
    int mWebStatus;
    bool mInvalidInput, mHasConfig, mIsStarted, mSaved;
    Str mInvalidFieldname;
    ConfigReader *mConfigReader;
    HiveConfig mConfig;
    Str mConfigFilename, mCurrentLine;
    class WiFiServer *mServer;
    class WiFiClient *mClient;
    const WifiUtils::Context *mCtxt;

  ProvisionImp(const char *resetCause, const char *versionId, const char *configFilename)
    : mConfig(resetCause, versionId), mConfigFilename(configFilename), 
      mServer(0), mClient(0), mConfigReader(0),
      mCtxt(0), 
      mNextActionTime(0), mMajorState(NOOP), mMinorState(0), mInvalidInput(false), mInvalidFieldname("unknown"),
      mHasConfig(false), mIsStarted(false), mSaved(false)
  {
  }

  ~ProvisionImp()
  {
      delete mServer;
      delete mClient;
      delete mConfigReader;
      delete mCtxt;
  }

  void start(bool ignoreConfigValidity = false)
  {
      TF("ProvisionImp::start");
      mNextActionTime = 0;
      mInvalidInput = mHasConfig = mIsStarted = mSaved = false;
      if (ignoreConfigValidity) {
	  TRACE("Ignoring the filed config's validity; setting to default");
	  mConfig.setDefault();
	  mMajorState = START_AP;
	  mMinorState = INIT;
	  mIsStarted = true;
      } else {
	  mMajorState = LOAD_CONFIG;
	  mMinorState = INIT;
	  mIsStarted = true;
      }
  }

  void stop()
  {
      if (mMajorState != STOP_AP) {
	  mMajorState = STOP_AP;
	  mMinorState = INIT;
      }
  }

  bool loop(unsigned long now, Mutex *wifiMutex);
  
  bool apLoop(unsigned long now, Mutex *wifiMutex);
  bool stopLoop(unsigned long now, Mutex *wifiMutex);
  
  bool loadLoop(unsigned long now);

  void sendOk(WiFiClient &client);
  void sendPage(WiFiClient &client, const char *ssid, const char *pswd,
		const char *dbHost, int dbPort, bool isSsl,
		const char *dbUser, const char *dbPswd);
  void parseRequest(const Str &url);

  // search for a string of the form key=value in
  // a string that looks like q?xyz=abc&uvw=defgh HTTP/1.1\r\n
  //
  // The returned value is stored in retval.
  //
  // Return LENGTH of found value of seeked parameter. this can return 0 if parameter was there but the value was missing!
  static int getKeyVal(const char *key, const char *str, Str *retval);
  
};
  

static Provision *s_singleton = NULL;

#define SSID "HIVEWIZ_192.168.1.1"


bool ProvisionImp::loadLoop(unsigned long now)
{
    TF("ProvisionImp::loadLoop");
    TRACE("entry");
    
    bool shouldReturn = true; // very few cases shouldn't return, so make true the default
    switch (mMinorState) {
    case INIT: {
        mConfigReader = new ConfigReader(mConfigFilename.c_str());
	mConfigReader->setup();
	mMinorState = LOADING;
    }
	break;
	
    case LOADING: {
        bool callConfigReaderAgain = mConfigReader->loop();
	if (!callConfigReaderAgain) {
	    if (mConfigReader->hasConfig()) {
	        if (HiveConfig::docIsValidForConfig(mConfigReader->getConfig())) {
		    mConfig.setDoc(mConfigReader->getConfig());
		    
		    Str dump;
		    TRACE2("Have a local configuration: ", CouchUtils::toString(mConfig.getDoc(), &dump));
		    mHasConfig = mConfig.isValid();
		    if (!mHasConfig) {
		        TRACE("Invalid local config loaded");
		    }
		} else {
		    TRACE("Loaded config is invalid; setting to default");  
		    Str dump;
		    TRACE2("Default configuration: ", CouchUtils::toString(mConfig.getDoc(), &dump));
		    mConfig.setDefault();
		}
	    } else {
	        assert(mConfigReader->hasConfig(), "Couldn't load config");
	    }
	    delete mConfigReader;
	    mConfigReader = NULL;
	    mMajorState = mHasConfig ? DONE : START_AP;
	    mMinorState = INIT;
	} else {
	    TRACE("configReader wants me to call it back");
	}
    }
	break;

    default:
        TRACE2("Shouldn't get here; mState: ", mMinorState);
	assert(false, "Shouldn't get here");
    }

    mNextActionTime = now + 10l;
    return shouldReturn;
}


bool ProvisionImp::loop(unsigned long now, Mutex *wifiMutex)
{
    TF("ProvisionImp::loop");
    
    switch (mMajorState) {
    case NOOP: return true;
    case LOAD_CONFIG: return loadLoop(now);
    case START_AP: return apLoop(now, wifiMutex);
    case STOP_AP:  return stopLoop(now, wifiMutex);
    case DONE: return false;
    default:
      TRACE2("Shouldn't get here; mMajorState: ", mMajorState);
      assert(false, "SHouldn't get here");
    }
}




Provision::Provision(const char *resetCause, const char *versionId, const char *configFilename, unsigned long now)
  : mImp(new ProvisionImp(resetCause, versionId, configFilename))
{
    TF("Provision::Provision");
    
    if (s_singleton != NULL) {
        ERR("Provision class violated implied singleton rule");
    }
}


Provision::~Provision()
{
    delete mImp;
    s_singleton = NULL;
}


bool Provision::hasConfig() const
{
    return mImp->mHasConfig;
}


bool Provision::isStarted() const
{
    return mImp->mIsStarted;
}


const HiveConfig &Provision::getConfig() const {return mImp->mConfig;}

void Provision::start()
{
    mImp->start();
}


void Provision::forcedStart()
{
    mImp->start(true);
}


void Provision::stop()
{
    mImp->stop();
}


bool Provision::loop(unsigned long now, Mutex *wifiMutex)
{
    if (now > mImp->mNextActionTime)
        return mImp->loop(now, wifiMutex);
}


static
void reportConnectedDevice(WiFiClass &wifi)
{
#ifndef NDEBUG
    byte remoteMac[6];
  
    Serial.print("Device connected to AP, MAC address: ");
    wifi.APClientMacAddress(remoteMac);
    Serial.print(remoteMac[5], HEX);
    Serial.print(":");
    Serial.print(remoteMac[4], HEX);
    Serial.print(":");
    Serial.print(remoteMac[3], HEX);
    Serial.print(":");
    Serial.print(remoteMac[2], HEX);
    Serial.print(":");
    Serial.print(remoteMac[1], HEX);
    Serial.print(":");
    Serial.println(remoteMac[0], HEX);
#endif
}


bool ProvisionImp::apLoop(unsigned long now, Mutex *wifiMutex)
{
    TF("ProvisionImp::apLoop");

    bool callMeBack = true; // very few cases shouldn't return, so make true the default
	
    if (wifiMutex->own(this)) {
	switch (mMinorState) {
	case INIT: {
	    TRACE("INIT");
	    mCtxt = new WifiUtils::Context();
	    
	    int status = mCtxt->getWifi().beginAP(SSID);
	    assert(status == WL_AP_LISTENING, "Couldn't initiate AP");
	
	    // wait 2 seconds for connection:
	    mDelayCnt = 0;

	    mMinorState = DELAY;
	    mPostDelayMinorState = START_WEB_SERVICE;
	}
	  break;

	case DELAY: {
	    //TRACE("DELAY");
	    mDelayCnt++;
	    mNextActionTime = now + 200l;
	    if (mDelayCnt > 20) {
	        TRACE("Done delay");
	        // 10*200ms == 2s -- done delay
	        mMinorState = mPostDelayMinorState;
	    }
	}
	  break;

	case START_WEB_SERVICE: {
	    TRACE("START_WEB_SERVICE");
	    mServer = new WiFiServer(80);
	    mServer->begin();
	    mMinorState = WEB_LISTEN;
	}
	  break;

	case WEB_LISTEN: {
	    // compare the previous status to the current status
	    if (mWebStatus != WiFi.status()) {
	        // it has changed update the variable
	        mWebStatus = WiFi.status();

		if (mWebStatus == WL_AP_CONNECTED) {
		    // a device has connected to the AP
		    reportConnectedDevice(mCtxt->getWifi());
		} else {
		    // a device has disconnected from the AP, and we are back in listening mode
		    Serial.println("Device disconnected from AP");
		}
	    }

	    WiFiClient client = mServer->available();
	    if (client) {                               // if you get a client,
	        mMinorState = WEB_CLIENT;
		mClient = new WiFiClient(client);
		mCurrentLine = "";                // to hold the incoming request from client
	        TRACE("new client");
	    }
	}
	  break;

	case WEB_CLIENT: {
	    if (mClient->connected()) {
	        if (mClient->available()) {
		    char c = mClient->read();       // read a byte, then
		    D(c);                           // print it out the serial monitor
		    if (c == '\n') {                // if the byte is a newline character
		        // if the current line is blank, you got two newline characters in a row.
		        // that's the end of the client HTTP request, so send a response:
		        if (mCurrentLine.length() == 0) {
			    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
			    // and a content-type so the client knows what's coming, then a blank line:
			    sendOk(*mClient);

			    sendPage(*mClient, mConfig.getSSID(), mConfig.getPSWD(),
				     mConfig.getDbHost(), mConfig.getDbPort(), mConfig.isSSL(),
				     mConfig.getDbUser(), mConfig.getDbPswd());
				
			    // close the connection:
			    mClient->stop();
			    delete mClient;
			    mClient = 0;
			    DL("client disconnected");
			    mMinorState = WEB_LISTEN;
			    delay(10);
			} else {      // if you got a newline, then clear currentLine:
			    parseRequest(mCurrentLine);
			    mCurrentLine = "";
			}
		    } else if (c != '\r') {    // if you got anything else but a carriage return character,
		        mCurrentLine += c;      // add it to the end of the currentLine
		    }
		}
	    }
	}
	  break;

	default: assert(false, "invalid state");
	}
    }

    return callMeBack;
}


bool ProvisionImp::stopLoop(unsigned long now, Mutex *wifiMutex)
{
    TF("ProvisionImp::apLoop");

    bool callMeBack = true; // very few cases shouldn't return, so make true the default
	
    if (wifiMutex->own(this)) {
	switch (mMinorState) {
	case INIT: {
	    TRACE("INIT");

	    // wait 2 seconds for any activity to complete
	    mDelayCnt = 0;
	    mMinorState = DELAY;
	    mPostDelayMinorState = SHUTDOWN;
	}
	  break;
	  
	case DELAY: {
	    //TRACE("DELAY");
	    mDelayCnt++;
	    mNextActionTime = now + 200l;
	    if (mDelayCnt > 20) {
	        TRACE("Done delay");
	        // 10*200ms == 2s -- done delay
	        mMinorState = mPostDelayMinorState;
	    }
	}
	  break;

	case SHUTDOWN: {
	    TRACE("SHUTDOWN");
	    if (mClient != NULL) {
	        mClient->stop();
		delete mClient;
		mClient = NULL;
	    }
	    if (mServer != NULL) {
	        delete mServer;
		mServer = NULL;
	    }
	    if (mCtxt != NULL) {
	        delete mCtxt;
		mCtxt = NULL;
	    }
	    callMeBack = false;
	    wifiMutex->release(this);
	    delay(10);
	    mIsStarted = false;
	}
	  break;
	}
    }

    return callMeBack;
}



/* STATIC */
void ProvisionImp::sendPage(WiFiClient &client,
			    const char *ssid, const char *pswd,
			    const char *dbHost, int dbPort, bool isSsl,
			    const char *dbUser, const char *dbPswd)
{
    const char *pageStart = "<html><head><title>Hivewiz Provisioning</title><style>body{font-family: Arial}</style></head><body><form method=\"get\" action=\"/\"><input type=\"hidden\" name=\"save\" value=\"1\">\r\n";
    const char *pageEnd = "</form><hr>\r\n</body></html>\r\n";

    // html pages (NOTE: make sure you don't have the '{' without the closing '}' !
    const char *pageSet = "<h2>Hivewiz Settings</h2><input type=\"hidden\" name=\"page\" value=\"wifi\"><table border=\"0\"><tr><td><b>SSID:</b></td><td><input type=\"text\" name=\"ssid\" value=\"{ssid}\" size=\"40\"></td></tr><tr><td><b>Password:</b></td><td><input type=\"text\" name=\"pass\" value=\"******\" size=\"40\"></td></tr><tr><td><b>CouchDB url:</b></td><td><input type=\"text\" name=\"couchDbHost\" value=\"{couchDbHost}\" size=\"40\"></td></tr><tr><td><b>CouchDB port:</b></td><td><input type=\"text\" name=\"couchDbPort\" value=\"{couchDbPort}\" size=\"40\"></td></tr><tr><td><b>CouchDB uses ssl (y/n):</b></td><td><input type=\"text\" name=\"isSsl\" value=\"{isSsl}\" size=\"40\"></td></tr><tr><td><b>CouchDB Username:</b></td><td><input type=\"text\" name=\"couchDbUser\" value=\"{couchDbUser}\" size=\"40\"></td></tr><tr><td><b>CouchDB Password:</b></td><td><input type=\"text\" name=\"couchDbPswd\" value=\"{couchDbPswd}\" size=\"40\"></td></tr><tr><td><b>Status:</b></td></tr><td>{status} <a href=\"?page=wifi\">[refresh]</a></td></tr><tr><td></td><td><input type=\"submit\" value=\"Save hi\"></td></tr><tr></tr></table>\r\n";
    const char *pageSavedInfo = "<br><b style=\"color: green\">Settings Saved!</b>\r\n";

    client.print(pageStart);
    D(pageStart);

    // wifi settings page
    Str buf1, buf2;
    const char *r = StringUtils::replace(&buf1, pageSet, "{ssid}", ssid);
    r = StringUtils::replace(&buf2, r, "{pass}", pswd);
    buf1.clear();
    r = StringUtils::replace(&buf1, r, "{couchDbHost}", dbHost);
    buf2.clear();
    r = StringUtils::replace(&buf2, r, "{couchDbPort}", Str().append(dbPort).c_str());
    buf1.clear();
    r = StringUtils::replace(&buf1, r, "{isSsl}", isSsl?"y":"n");
    buf2.clear();
    r = StringUtils::replace(&buf2, r, "{couchDbUser}", dbUser);
    buf1.clear();
    r = StringUtils::replace(&buf1, r, "{couchDbPswd}", dbPswd);

    Str statMsg = "Loaded Config";
    if (mInvalidInput) {
        statMsg = "Invalid value: ";
	statMsg.append(mInvalidFieldname);
    }

    buf2.clear();
    r = StringUtils::replace(&buf2, r, "{status}", statMsg.c_str());
    if (mSaved) {
        buf1 = r;
	r = buf1.append(pageSavedInfo).c_str();
	mSaved = false;
    }

    client.print(r);
    D(r);
    
    // page footer
    client.print(pageEnd);
    D(pageEnd);
    client.println();
    DL("");
}


/* STATIC */
void ProvisionImp::sendOk(WiFiClient &client)
{
    client.println("HTTP/1.1 200 OK");
    DL("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    DL("Content-type:text/html");
    client.println();
    DL("");
}


void ProvisionImp::parseRequest(const Str &line)
{
    TF("ProvisionImp::parseRequest");
    TRACE2("line to parse: ", line.c_str());

    Str save;
    getKeyVal("save", line.c_str(), &save);
    TRACE2("save: ", save.c_str());
    
    // saving data?
    if( save.c_str()[0] == '1' ) {
        TRACE("Saving data");
	mSaved = true;

	mInvalidInput = false;
	
        // copy parameters from URL GET to actual destination in structure
        CouchUtils::Doc newDoc;
	
        Str buf;
	getKeyVal("ssid", line.c_str(), &buf); //32 -- from sysroot/usr/include/user_interface.h
	newDoc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::SsidProperty, CouchUtils::Item(buf)));
	
	// set password? we have to hide it...
	buf.clear();
	getKeyVal("pass", line.c_str(), &buf);
	if( !buf.equals("******") )
	    newDoc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::SsidPswdProperty,
							      CouchUtils::Item(buf)));

	buf.clear();
	getKeyVal("couchDbHost", line.c_str(), &buf);
	newDoc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbHostProperty, CouchUtils::Item(buf)));

	buf.clear();
	getKeyVal("couchDbUser", line.c_str(), &buf);
	newDoc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbUserProperty, CouchUtils::Item(buf)));

	buf.clear();
	getKeyVal("couchDbPswd", line.c_str(), &buf);
	newDoc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbPswdProperty, CouchUtils::Item(buf)));

	buf.clear();
	getKeyVal("couchDbPort", line.c_str(), &buf);
	newDoc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::DbPortProperty, CouchUtils::Item(buf)));
	if (!StringUtils::isNumber(buf.c_str())) {
	    TRACE2("couchDbPort failed isNumber test: ", buf.c_str());
	    mInvalidInput = true;
	    mInvalidFieldname = "CouchDbPort";
	}

	buf.clear();
	getKeyVal("isSsl", line.c_str(), &buf);
	Str buf2 = buf.tolower();
	bool isValidFlag = buf2.equals("y") || buf2.equals("n") || 
	  buf2.equals("true") || buf2.equals("false") ||
	  buf2.equals("yes") || buf2.equals("no");
	bool isSsl = buf2.equals("y") || buf2.equals("true") || buf2.equals("yes");
	newDoc.addNameValue(new CouchUtils::NameValuePair(HiveConfig::IsSslProperty,
							  CouchUtils::Item(isSsl ? "true" : "false")));
	if (!isValidFlag) {
	    TRACE2("IsSSL property validity check failed: ", buf.c_str());
	    mInvalidInput = true;
	    mInvalidFieldname = "uses ssl";
	}
	
	const CouchUtils::Doc &prevDoc = mConfig.getDoc();
	for (int i = 0; i < prevDoc.getSz(); i++) {
	    if (newDoc.lookup(prevDoc[i].getName()) < 0)
	        newDoc.addNameValue(new CouchUtils::NameValuePair(prevDoc[i]));
	}

	if (!mInvalidInput && !HiveConfig::docIsValidForConfig(newDoc)) {
	    mInvalidInput = true;
	    mInvalidFieldname = "unknown";
	    mHasConfig = false;
	} else {
	    mConfig.setDoc(newDoc);
	    mHasConfig = mConfig.isValid();
	    if (mHasConfig) {
	        ConfigWriter writer(mConfigFilename.c_str(), newDoc);
		TRACE("Writing config out to file...");
		while (writer.loop()) {
		    TRACE("Writing config out to file...");
		}
	    }
	}

	Str dump;
	TRACE2("updated configuration: ", CouchUtils::toString(mConfig.getDoc(), &dump));
    }
}



// search for a string of the form key=value in
// a string that looks like q?xyz=abc&uvw=defgh HTTP/1.1\r\n
//
// The returned value is stored in retval.
//
// Return LENGTH of found value of seeked parameter. this can return 0 if parameter was there but the value was missing!
/* STATIC */
int ProvisionImp::getKeyVal(const char *key, const char *str, Str *retval)
{
    TF("ProvisionImp::getKeyVal");
    
    unsigned char found = 0;
    const char *keyptr = key;
    char prev_char = '\0';
    *retval = "";
    
    while( *str && *str!='\r' && *str!='\n' && !found ) {
        // GET /whatever?page=wifi&action=search HTTP/1.1\r\n
        if(*str == *keyptr) {
	    // At the beginning of the key we must check if this is the start of the key otherwise we will
	    // match on 'foobar' when only looking for 'bar', by andras tucsni, modified by trax
	    if(keyptr == key && !( prev_char == '?' || prev_char == '&' ) ) {
	        // trax: accessing (str-1) can be a problem if the incoming string starts with the key itself!
	        str++;
		continue;
	    }
			
	    keyptr++;

	    if (*keyptr == '\0') {
		str++;
		keyptr = key;
		if (*str == '=')
		    found = 1;
	    }
	} else {
	    keyptr = key;
	}
	prev_char = *str;
	str++;
    }

    if(found == 1) {
	found = 0;
	
	// copy the value to a buffer and terminate it with '\0'
	while( *str && *str!='\r' && *str!='\n' && *str!=' ' && *str!='&' ) {
	    retval->append(*str);
	    str++;
	    found++;
	}
    }
    
    return found;
}

