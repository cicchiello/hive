#ifndef mywifi_h
#define mywifi_h

#include <WiFi101.h>

class MyWiFi : public WiFiClass {
 public:
    MyWiFi();

    uint8_t beginNoWait(const char *ssid);
    uint8_t beginNoWait(const char *ssid, const char *key);

    uint8_t connectCheck();

    void connectionFailed();

    using WiFiClass::startProvision;
    
int testHostByName(const char* aHostname, IPAddress& aResult);
    
    enum DNS_State {DNS_FAILED, DNS_RETRY, DNS_SUCCESS};
    // return 1 on success; 0 to suggest a retry later
    DNS_State dnsNoWait(const char* aHostname, IPAddress& aResult);
    
    // return 1 on success; 0 to suggest a retry later
    DNS_State dnsCheck(IPAddress &aResult);

    void dnsFailed();
    
 private:
    uint8_t startConnectNoWait(const char *ssid, uint8_t u8SecType, const void *pvAuthInfo);
};

#endif
