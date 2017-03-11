#include <wifiutils.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <Trace.h>

#include <platformutils.h>


#define nibHigh(b) (((b)&0xf0)>>4)
#define nibLow(b) ((b)&0x0f)
#define hex2asc(nibble) (((nibble)>9) ? (nibble-10)+'a':(nibble)+'0')

// The SPI pins of the WINC1500 (SCK, MOSI, MISO) should be
// connected to the hardware SPI port of the Arduino.
//
// I have yet to determine/confirm the pinouts on the feather M0+
//

static MyWiFi myWiFi;
WiFiClass &WiFi = myWiFi;

BufferedWiFiClient client;


// Or just use hardware SPI (SCK/MOSI/MISO) and defaults, SS -> #10, INT -> #7, RST -> #5, EN -> 3-5V
//Adafruit_WINC1500 WiFi;


// there can only be one WifiUtils::Context at a time in order to ensure proper connect/disconnect
// sequencing -- so assert it with code
static WifiUtils::Context *theSingleContext = NULL;


BufferedWiFiClient::BufferedWiFiClient()
  : mLoc(0)
{
    memset(mBuf, 0, BUFSZ);
}

size_t BufferedWiFiClient::write(const uint8_t *buf, size_t size)
{
    assert(mLoc+size<BUFSZ, "buffer exceeded");
    memcpy(mBuf+mLoc, buf, size);
    mLoc += size;
    return size;
}

bool BufferedWiFiClient::flushOut(int *remaining)
{
    bool r = flushNoBlock((const uint8_t*) mBuf, mLoc, remaining);
    if (r) {
        mLoc = 0;
	memset(mBuf, 0, mLoc);
    }
    return r;
}

						 

WifiUtils::Context::Context()
{
    TF("WifiUtils::Context::Context");
    TRACE("entry");
    
    assert(theSingleContext == NULL, "There can only be one WifiUtils::Context object at a time");
    
    static bool s_first = true;
    if (s_first) {
        myWiFi.setPins(WINC_CS, WINC_IRQ, WINC_RST, WINC_EN);

        pinMode(WINC_EN, OUTPUT);
	digitalWrite(WINC_RST, LOW);
	delay(10);
	digitalWrite(WINC_RST, HIGH);
	delay(10);

        s_first = false;
    }
    
    digitalWrite(WINC_EN, HIGH);

    theSingleContext = this;

    TRACE("exit");
}

WifiUtils::Context::~Context()
{
    DL("WifiUtils::Context DTOR");
    reset();

//    getWifi().disconnect();

//    digitalWrite(WINC_EN, LOW);
//    delay(10);

    theSingleContext = NULL;
}


void WifiUtils::Context::reset() const
{
    DL("WifiUtils::Context::reset; stopping the client");
    getClient().stop();
    DL("WifiUtils::Context::reset; client stopped");
}


Adafruit_WINC1500 & WifiUtils::Context::getWifi() const 
{
    return myWiFi;
}


/* STATIC */
BufferedWiFiClient &WifiUtils::Context::getClient() const
{
    return client;
}


/* STATIC */
void WifiUtils::getMacAddress(const Adafruit_WINC1500 &wifi, char *buf) {
    // the MAC address of your Wifi shield
    unsigned char mac[6];

    Adafruit_WINC1500 *nonconstWifi = (Adafruit_WINC1500*) &wifi;
    
    // convert MAC address to string
    nonconstWifi->macAddress(mac);

    buf[0] = hex2asc(nibHigh(mac[5]));
    buf[1] = hex2asc(nibLow(mac[5]));

    buf[2] = ':';
  
    buf[3] = hex2asc(nibHigh(mac[4]));
    buf[4] = hex2asc(nibLow(mac[4]));

    buf[5] = ':';

    buf[6] = hex2asc(nibHigh(mac[3]));
    buf[7] = hex2asc(nibLow(mac[3]));

    buf[8] = ':';

    buf[9] = hex2asc(nibHigh(mac[2]));
    buf[10] = hex2asc(nibLow(mac[2]));

    buf[11] = ':';

    buf[12] = hex2asc(nibHigh(mac[1]));
    buf[13] = hex2asc(nibLow(mac[1]));

    buf[14] = ':';

    buf[15] = hex2asc(nibHigh(mac[0]));
    buf[16] = hex2asc(nibLow(mac[0]));

    buf[17] = 0;
}


/* STATIC */
void WifiUtils::getNetworks(const Adafruit_WINC1500 &wifi, SSID **networks, int *numNetworks)
{
    Adafruit_WINC1500 *nonconstWifi = (Adafruit_WINC1500*) &wifi;
    
    int numSsid = nonconstWifi->scanNetworks();
    if (numSsid > 0) {
        *networks = new WifiUtils::SSID[numSsid];
	*numNetworks = numSsid;
	for (int i = 0; i < numSsid; i++) {
	    strcpy((*networks)[i].name, nonconstWifi->SSID(i));
	    (*networks)[i].strength = nonconstWifi->RSSI(i);
	    (*networks)[i].encryptionType = nonconstWifi->encryptionType(i);
	}
    }
}

/* STATIC */
const char* WifiUtils::encryptionTypename(char thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
  case ENC_TYPE_WEP: return "WEP";
  case ENC_TYPE_TKIP: return "WPA";
  case ENC_TYPE_CCMP: return "WPA2";
  case ENC_TYPE_NONE: return "None";
  case ENC_TYPE_AUTO: return "Auto";
  default: return "<unknown>";
  }
}


/* STATIC */
WifiUtils::ConnectorStatus WifiUtils::connector(const WifiUtils::Context &ctxt,
						const char *ssid, const char *pswd,
						int *connectorState, unsigned long *callMeBackIn_ms)
{
    TF("WifiUtils::connector");
    TRACE("entry");

    uint8_t r;
    if (*connectorState >= 30) {
        TRACE("connect attempt failed; timeout");
        // couldn't connect
	ctxt.getWifi().connectionFailed();
        return ConnectTimeout;
    } else if (*connectorState == 0) {
        TRACE2("Attempting to connect to SSID/PWD: ",
	       Str(ssid).append2("/").append2(pswd == NULL ? "<null>" : pswd).c_str());

        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
	r = pswd == NULL ? ctxt.getWifi().begin(ssid) : ctxt.getWifi().beginNoWait(ssid, pswd);
    } else {
        r = ctxt.getWifi().connectCheck();
    }
    
    (*connectorState)++;
    *callMeBackIn_ms = 500l;
	
    // re-check status for ~10s
    switch (r) {
    case WL_CONNECTED:
        if (ctxt.getWifi().localIP() == INADDR_NONE) {
	    TRACE("have WL_CONNECTED but not IP yet");
	    return ConnectRetry;
	} else { 
	    TRACE("Connected...");
	    return ConnectSucceed;
	}
    case WL_CONNECT_FAILED: 
    case WL_CONNECTION_LOST:
    case WL_DISCONNECTED:
    case WL_SCAN_COMPLETED:
    case WL_NO_SSID_AVAIL:
    case WL_IDLE_STATUS:
    case WL_NO_SHIELD:
        TRACE2("status == ", r);
	return ConnectRetry;
    default:
        TRACE2("unknown state; status == ", r);
	return ConnectRetry;
    }
}


WifiUtils::DisconnectorStatus WifiUtils::disconnector(const WifiUtils::Context &ctxt,
						      int *disconnectorState,
						      unsigned long *callMeBackIn_ms)
{
    TF("WifiUtils::disconnector");
    if (*disconnectorState == 0) {
        PH("Attempting to disconnect;");
	ctxt.getWifi().disconnect();
	    
	*disconnectorState = 1;
	*callMeBackIn_ms = 500l;  // 1/2 second
	return DisconnectRetry;
    } else if (*disconnectorState < 20) {
        // re-check status for ~10s
        uint8_t stat = ctxt.getWifi().status();
        switch (stat) {
	case WL_CONNECTION_LOST:
	case WL_IDLE_STATUS:
	case WL_DISCONNECTED:
	    PH("Disconnected");
	    return DisconnectSucceed;
	case WL_CONNECTED: 
	case WL_CONNECT_FAILED: 
	case WL_SCAN_COMPLETED:
	case WL_NO_SSID_AVAIL:
	case WL_NO_SHIELD:
	default:
	    PH("WifiUtils::disconnector: status == ");
	    PL(stat);
	    (*disconnectorState)++;
	    *callMeBackIn_ms = 500l;
	    return DisconnectRetry;
	}
    } else {
        // couldn't connect
        return DisconnectTimeout;
    }
}


/* STATIC */
void WifiUtils::printWifiStatus() {
    TF("WifiUtils::printWifiStatus");
    
    // print the SSID of the network you're attached to:
    P("SSID: \'");
    P(WiFi.SSID());
    PL("\'");

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    P("IP Address: ");
    PL(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    P("signal strength (RSSI):");
    P(rssi);
    PL(" dBm");

    // print local subnet mask
    uint32_t sn = WiFi.subnetMask();
    P("local subnet mask: ");
    Serial.println(sn, HEX);
}



#if MIGHT_NEED_IT
static void URLEncode(const char* msg, Str *buf)
{
    // according to https://tools.ietf.org/html/rfc3986, the following are the legal characters within a url
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;="
    // (however, any that may be context specific are also escaped here)
    static const char *hex = "0123456789abcdef";

    int i = 0;
    while (*msg!='\0'){
        if( ('a' <= *msg && *msg <= 'z') ||
	    ('A' <= *msg && *msg <= 'Z') ||
	    ('0' <= *msg && *msg <= '9') ||
	    ('_' == *msg) || ('.' == *msg) || ('-' == *msg)) {
	    buf->add(*msg);
        } else {
	    buf->add('%');
	    buf->add(hex[*msg >> 4]);
	    buf->add(hex[*msg & 0x0f]);
        }
        msg++;
    }
    buf->add(0);
}
#endif

