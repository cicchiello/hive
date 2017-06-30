#ifndef http_sslbinaryput_h
#define http_sslbinaryput_h

#include <http_binaryput.h>

class HttpSSLBinaryPut : public HttpBinaryPut {
 public:
   HttpSSLBinaryPut(const char *ssid, const char *ssidPswd, 
		    const char *host, int port,
		    const char *dbUser, const char *dbPswd, 
		    HttpDataProvider *provider, const char *contentType, const char *urlPieces[])
     : HttpBinaryPut(ssid, ssidPswd, host, port, dbUser, dbPswd, true, provider, contentType, urlPieces) {}
   virtual ~HttpSSLBinaryPut();

 protected:
   HttpSSLBinaryPut(const HttpSSLBinaryPut &); //intentionally unimplemented
};

#endif
