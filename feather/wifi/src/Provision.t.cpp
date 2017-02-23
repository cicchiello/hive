#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>
#define CHKPT(msg) HivePlatform::singleton()->trace(msg)
#define ERR(msg) HivePlatform::singleton()->error(msg)

#include <version_id.h>

#include <wifiutils.h>

#include <Provision.h>
#include <couchutils.h>
#include <Mutex.h>
#include <hiveconfig.h>
#include <AppChannel.h>

#include <MyWiFi.h>


#define CONFIG_FILENAME         "/CONFIG.CFG"
  
static const char *ResetCause = "unknown";
static Provision *s_provisioner = NULL;
static AppChannel *s_appChannel = NULL;
static bool s_isOnline = false;

static Mutex sWifiMutex;

static WifiUtils::Context *ctxt = NULL;


/**************************************************************************/
/*!
    @brief  Sets up the HW (this function is called automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
    // capture the reason for previous reset so I can inform the app
    ResetCause = HivePlatform::singleton()->getResetCause(); 
    
    delay(500);

#ifndef HEADLESS
    while (!Serial);  // required for Flora & Micro
    delay(500);
    Serial.begin(115200);
#endif
  
    P("ResetCause: "); PL(ResetCause);
    
    pinMode(19, INPUT);               // used for preventing runnaway on reset
    while (digitalRead(19) == HIGH) {
        delay(500);
	PL("within the runnaway prevention loop");
    }
    PL("digitalRead(19) returned LOW");

    //pinMode(5, OUTPUT);         // for debugging: used to indicate ISR invocations for motor drivers
    
    delay(500);
    
    PL("Hive Controller debug console");
    PL("---------------------------------------");

    ctxt = new WifiUtils::Context();

    uint8_t mac[6];
    char provSsid[13];

    // get MAC address for provisioning SSID
    ctxt->getWifi().macAddress(mac);
    sprintf(provSsid, "Hivewiz-%.2X%2X", mac[1], mac[0]);

    HivePlatform::nonConstSingleton()->startWDT();
  
    s_provisioner = new Provision(ResetCause, VERSION, CONFIG_FILENAME, millis());
    
    CHKPT("setup done");
    PL("Setup done...");
    
    pinMode(10, OUTPUT);  // set the SPI_CS pin as output
}

void printWiFiStatus();

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
    TF("::loop; PROVISION");
    unsigned long now = millis();

    HivePlatform::nonConstSingleton()->clearWDT();
    if (s_appChannel == NULL) {
	if (s_provisioner->hasConfig() && s_provisioner->isStarted()) {
	    //TRACE("Has a valid config; stopping the provisioner");
	    s_provisioner->stop();
	    delay(500l);
	} else if (s_provisioner->hasConfig() && !s_provisioner->isStarted()) {
	    TRACE("Has a valid config and provisioner is stopped; starting AppChannel");
	    TRACE("If AppChannel cannot connect within 60s, will revert to Provisioning");
	    TRACE2("now: ", now);
	    s_appChannel = new AppChannel(s_provisioner->getConfig(), now);
	} else if (!s_provisioner->isStarted()) {
	    TRACE("No valid config; starting the Provisioner");
	    s_provisioner->start();
	}
    } else {
        if (s_appChannel->loop(now, &sWifiMutex)) {
	    if (s_appChannel->haveMessage()) {
	    }
	}
	if (!s_isOnline && s_appChannel->isOnline()) {
	    TRACE("Detected AppChannel online");
	    s_isOnline = true;
	} else if (s_isOnline && !s_appChannel->isOnline()) {
	    TRACE("Detected AppChannel offline");
	    s_isOnline = false;
	    s_provisioner->forcedStart();
	    delete s_appChannel;
	    s_appChannel = NULL;
	} else if (!s_isOnline &&
		   (now > s_appChannel->getCreationTime() + 60*1000l) &&
		   sWifiMutex.isAvailable()) {
	    TRACE("Detected AppChannel offline for more than 60s; initiating forced Provision mode");
	    TRACE("Detected AppChannel offline; initiating forced Provision mode");
	    assert(sWifiMutex.isAvailable(), "sWifiMutex.isAvailable()");
	    s_isOnline = false;
	    s_provisioner->forcedStart();
	    delete s_appChannel;
	    s_appChannel = NULL;
	}
    }

    if (s_provisioner->isStarted() && s_provisioner->isItTimeYet(now)) {
        //TRACE("s_provisioner says it's time");
        if (!s_provisioner->loop(now, &sWifiMutex)) {
	    Str dump;
	    TRACE2("Using Config: ", CouchUtils::toString(s_provisioner->getConfig().getDoc(), &dump));
	}
    }
}


void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(ctxt->getWifi().SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = ctxt->getWifi().localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = ctxt->getWifi().RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
