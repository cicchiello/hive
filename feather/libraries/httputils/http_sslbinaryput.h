#ifndef http_sslbinaryput_h
#define http_sslbinaryput_h

#include <http_binaryput.h>

class HttpSSLBinaryPut : public HttpBinaryPut {
 public:
   HttpSSLBinaryPut(const char *ssid, const char *ssidPswd, 
		    const char *host, int port, const char *page, const char *credentials,
		    HttpDataProvider *provider, const char *contentType)
     : HttpBinaryPut(ssid, ssidPswd, host, port, page, credentials, true, provider, contentType) {}
   HttpSSLBinaryPut(const char *ssid, const char *ssidPswd, 
		    const IPAddress &hostip, int port, const char *page, const char *credentials,
		    HttpDataProvider *provider, const char *contentType)
     : HttpBinaryPut(ssid, ssidPswd, hostip, port, page, credentials, true, provider, contentType) {}
   virtual ~HttpSSLBinaryPut();

 protected:
   HttpSSLBinaryPut(const HttpSSLBinaryPut &); //intentionally unimplemented
};

#endif
