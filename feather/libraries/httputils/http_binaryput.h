#ifndef http_binaryput_h
#define http_binaryput_h

#include <http_couchget.h>

class HttpDataProvider;

class HttpBinaryPut : public HttpCouchGet {
 public:
    HttpBinaryPut(const char *ssid, const char *ssidPswd, 
		  const char *host, int port, const char *page,
		  const char *dbUser, const char *dbPswd, 
		  bool isSSL, HttpDataProvider *provider, const char *contentType);
    HttpBinaryPut(const char *ssid, const char *ssidPswd, 
		  const IPAddress &hostip, int port, const char *page,
		  const char *dbUser, const char *dbPswd, 
		  bool isSSL, HttpDataProvider *provider, const char *contentType);
    virtual ~HttpBinaryPut();

    virtual EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);

    using HttpCouchGet::EventResult;
   
    using HttpCouchGet::getHeaderConsumer;
    using HttpCouchGet::getDoc;
    using HttpCouchGet::haveDoc;
    using HttpCouchGet::processEventResult;
    using HttpCouchGet::getFinalResult;

 protected:
    HttpBinaryPut(const HttpBinaryPut &); //intentionally unimplemented

    virtual void resetForRetry();
    
    void sendPUT(Stream &s, int contentLength) const;

    const Str m_contentType;
    
    int m_writtenCnt;
    HttpDataProvider *m_provider;
};

#endif
