#ifndef wifiutils_h
#define wifiutils_h


#include <MyWiFi.h>


// Define the WINC1500 board connections below.
// If you're following the Adafruit WINC1500 board
// guide you don't need to modify these:
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2     // or, tie EN to VCC

class MyWiFi;
class WiFiClient;
typedef MyWiFi Adafruit_WINC1500;
typedef WiFiClient Adafruit_WINC1500Client;

class BufferedWiFiClient : public WiFiClient {
 private:
  static const int BUFSZ = 1440;
  
  char mBuf[BUFSZ];
  int mLoc;
  
 public:
  BufferedWiFiClient();
  
  size_t write(const uint8_t *buf, size_t size);
  
  size_t write(const char *buf, size_t size) {return write((const uint8_t*)buf, size);}

  bool flushOut(int *remaining);
};


class WifiUtils {
  public:
  
    class Context {
    public:
      Context();
      ~Context();

      void reset() const;
      
      Adafruit_WINC1500 &getWifi() const;
      BufferedWiFiClient &getClient() const;
    };
    
    // buf must be at least 18 chars long
    static void getMacAddress(const Adafruit_WINC1500 &wifi, char *buf);

    struct SSID {
      char name[33];
      short strength;
      char encryptionType;
    };
    static void getNetworks(const Adafruit_WINC1500 &wifi, SSID **networks, int *numNetworks);

    static const char *encryptionTypename(char t);

    enum ConnectorStatus {
        ConnectRetry, ConnectSucceed, ConnectTimeout
    };
    static ConnectorStatus connector(const Context &ctxt, const char *ssid, const char *pswd,
				     int *connectorState, unsigned long *callMeBackIn_ms);

    enum DisconnectorStatus {
        DisconnectRetry, DisconnectSucceed, DisconnectTimeout
    };
    static DisconnectorStatus disconnector(const Context &ctxt, int *disconnectorState,
					   unsigned long *callMeBackIn_ms);

    static void printWifiStatus();

};

#endif
