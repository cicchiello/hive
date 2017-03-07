#include <MyWiFi.h>

#define NDEBUG

#include <Trace.h>


extern "C" {
  #include "bsp/include/nm_bsp.h"
  #include "bsp/include/nm_bsp_arduino.h"
  #include "socket/include/socket_buffer.h"
  #include "socket/include/m2m_socket_host_if.h"
  #include "driver/source/nmasic.h"
  #include "driver/include/m2m_periph.h"
}


MyWiFi::MyWiFi()
{
}

uint8_t MyWiFi::beginNoWait(const char *ssid, const char *key)
{
    TF("MyWiFi::beginNoWait");
    return startConnectNoWait(ssid, M2M_WIFI_SEC_WPA_PSK, key);
}

uint8_t MyWiFi::beginNoWait(const char *ssid)
{
	return startConnect(ssid, M2M_WIFI_SEC_OPEN, (void *)0);
}

uint8_t MyWiFi::startConnectNoWait(const char *ssid, uint8_t u8SecType, const void *pvAuthInfo)
{
    TF("MyWiFi::startConnectNoWait");

	if (!_init) {
		init();
	}
	
	// Connect to router:
	if (_dhcp) {
		_localip = 0;
		_submask = 0;
		_gateway = 0;
	}
	if (m2m_wifi_connect(ssid, strlen(ssid), u8SecType, pvAuthInfo, M2M_WIFI_CH_ALL) < 0) {
		_status = WL_CONNECT_FAILED;
		return _status;
	}
	_status = WL_IDLE_STATUS;
	_mode = WL_STA_MODE;

	// Wait for connection or timeout:
	unsigned long start = millis();

	TRACE("before first handle_events call");
	m2m_wifi_handle_events(NULL);
	
	memset(_ssid, 0, M2M_MAX_SSID_LEN);
	memcpy(_ssid, ssid, strlen(ssid));
	return _status;
}

uint8_t MyWiFi::connectCheck()
{
    TF("MyWiFi::connectCheck");
    
	TRACE("before another handle_events call");
	m2m_wifi_handle_events(NULL);
	
	return _status;
}

void MyWiFi::connectionFailed()
{
	if (!(_status & WL_CONNECTED)) {
		_mode = WL_RESET_MODE;
	}
}


MyWiFi::DNS_State MyWiFi::dnsNoWait(const char* aHostname, IPAddress& aResult)
{
	// check if aHostname is already an ipaddress
	if (aResult.fromString(aHostname)) {
		// if fromString returns true we have an IP address ready 
		return DNS_SUCCESS;

	} else {
		// Send DNS request:
		_resolve = 0;
		if (gethostbyname((uint8 *)aHostname) < 0) {
		    return DNS_FAILED;
		}

		m2m_wifi_handle_events(NULL);

		if (_resolve == 0) {
		    return DNS_RETRY;
		}

		aResult = _resolve;
		_resolve = 0;
		return DNS_SUCCESS;
	}
}

extern "C" {
static TraceScope *s1 = NULL;
static TraceScope *s2 = NULL;
static TraceScope *s3 = NULL;
static TraceScope *s4 = NULL;
static TraceScope *s5 = NULL;
static TraceScope *s6 = NULL;
static TraceScope *s7 = NULL;
static TraceScope *s8 = NULL;
static TraceScope *s9 = NULL;
static TraceScope *s10 = NULL;
static TraceScope *s11 = NULL;
void trace1CTOR() {s1 = new TraceScope("::trace1xTOR", __FILE__, __LINE__);}
void trace1DTOR() {delete s1; s1 = NULL;}
void trace2CTOR() {s2 = new TraceScope("::trace2xTOR", __FILE__, __LINE__);}
void trace2DTOR() {delete s2; s2 = NULL;}
void trace3CTOR() {s3 = new TraceScope("::trace3xTOR", __FILE__, __LINE__);}
void trace3DTOR() {delete s3; s3 = NULL;}
void trace4CTOR() {s4 = new TraceScope("::trace4xTOR", __FILE__, __LINE__);}
void trace4DTOR() {delete s4; s4 = NULL;}
void trace5CTOR() {s5 = new TraceScope("::trace5xTOR", __FILE__, __LINE__);}
void trace5DTOR() {delete s5; s5 = NULL;}
void trace6CTOR() {s6 = new TraceScope("::trace6xTOR", __FILE__, __LINE__);}
void trace6DTOR() {delete s6; s6 = NULL;}
void trace7CTOR() {s7 = new TraceScope("::trace7xTOR", __FILE__, __LINE__);}
void trace7DTOR() {delete s7; s7 = NULL;}
void trace8CTOR() {s8 = new TraceScope("::trace8xTOR", __FILE__, __LINE__);}
void trace8DTOR() {delete s8; s8 = NULL;}
void trace9CTOR() {s9 = new TraceScope("::trace9xTOR", __FILE__, __LINE__);}
void trace9DTOR() {delete s9; s9 = NULL;}
void trace10CTOR() {s10 = new TraceScope("::trace10xTOR", __FILE__, __LINE__);}
void trace10DTOR() {delete s10; s10 = NULL;}
void trace11CTOR() {s11 = new TraceScope("::trace11xTOR", __FILE__, __LINE__);}
void trace11DTOR() {delete s11; s11 = NULL;}
};

MyWiFi::DNS_State MyWiFi::dnsCheck(IPAddress& aResult)
{
    TF("MyWiFi::dnsCheck");
    
    m2m_wifi_handle_events(NULL);

    {
      TF("MyWiFi::dnsCheck; after m2m_wifi_handle_events call");
      
      if (_resolve == 0) {
        TF("MyWiFi::dnsCheck; _resolve==0");
        return DNS_RETRY;
      }

      aResult = _resolve;
      _resolve = 0;
      return DNS_SUCCESS;
    }
}

void MyWiFi::dnsFailed()
{
    _resolve = 0;
}



int MyWiFi::testHostByName(const char* aHostname, IPAddress& aResult)
{
	
	// check if aHostname is already an ipaddress
	if (aResult.fromString(aHostname)) {
		// if fromString returns true we have an IP address ready 
		return 1;

	} else {
		// Network led ON (rev A then rev B).
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);
	
		// Send DNS request:
		_resolve = 0;
		if (gethostbyname((uint8 *)aHostname) != 0) {
			// Network led OFF (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
			return 0;
		}

		// Wait for connection or timeout:
		unsigned long start = millis();
		while (_resolve == 0 && millis() - start < 20000) {
			m2m_wifi_handle_events(NULL);
		}

		// Network led OFF (rev A then rev B).
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);

		if (_resolve == 0) {
			return 0;
		}

		aResult = _resolve;
		_resolve = 0;
		return 1;
	}
}

