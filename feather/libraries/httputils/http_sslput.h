#ifndef http_sslput_h
#define http_sslput_h

#include <http_put.h>

class HttpSSLPut : public HttpPut {
 public:
   HttpSSLPut(const char *ssid, const char *ssidPswd, 
	      const char *host, int port, const char *page,
	      const char *dbUser, const char *dbPswd)
     : HttpPut(ssid, ssidPswd, host, port, page, dbUser, dbPswd) {}
   HttpSSLPut(const char *ssid, const char *ssidPswd, 
	      const IPAddress &hostip, int port, const char *page,
	      const char *dbUser, const char *dbPswd)
     : HttpPut(ssid, ssidPswd, hostip, port, page, dbUser, dbPswd) {}
   virtual ~HttpSSLPut();

 protected:
   HttpSSLPut(const HttpSSLPut &); //intentionally unimplemented
};

#endif
