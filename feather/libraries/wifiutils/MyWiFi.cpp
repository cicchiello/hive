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

MyWiFi::DNS_State MyWiFi::dnsCheck(IPAddress& aResult)
{
  Serial.println("dnsCheck; 1");
    m2m_wifi_handle_events(NULL);
  Serial.println("dnsCheck; 2");

    if (_resolve == 0) {
  Serial.println("dnsCheck; 3");
        return DNS_RETRY;
    }

  Serial.println("dnsCheck; 4");
    aResult = _resolve;
  Serial.println("dnsCheck; 5");
    _resolve = 0;
    return DNS_SUCCESS;
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

