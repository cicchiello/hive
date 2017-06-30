#ifndef http_sslput_h
#define http_sslput_h

#include <http_put.h>

class HttpSSLPut : public HttpPut {
 public:
   virtual ~HttpSSLPut();

 protected:
   HttpSSLPut(const HttpSSLPut &); //intentionally unimplemented

 private:
   HttpSSLPut(const char *ssid, const char *ssidPswd, 
	      const char *host, int port, const char *urlPieces[],
	      const char *dbUser, const char *dbPswd)
     : HttpPut(ssid, ssidPswd, host, port, urlPieces, dbUser, dbPswd) {}
};

#endif
