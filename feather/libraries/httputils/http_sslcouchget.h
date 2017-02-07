#ifndef http_sslcouchget_h
#define http_sslcouchget_h

#include <http_couchget.h>


class HttpSSLCouchGet: public HttpCouchGet {
 public:
   HttpSSLCouchGet(const char *ssid, const char *ssidPswd, 
		   const char *host, int port, const char *page, const char *credentials)
     : HttpCouchGet(ssid, ssidPswd, host, port, page, credentials)
       {}
   HttpSSLCouchGet(const char *ssid, const char *ssidPswd, 
		   const IPAddress &hostip, int port, const char *page, const char *credentials)
     : HttpCouchGet(ssid, ssidPswd, hostip, port, page, credentials)
       {}

 protected:
   HttpSSLCouchGet(const HttpSSLCouchGet &); //intentionally unimplemented
};

#endif
