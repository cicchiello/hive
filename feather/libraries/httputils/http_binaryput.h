#ifndef http_binaryput_h
#define http_binaryput_h

#include <http_couchget.h>

class HttpDataProvider;

class HttpBinaryPut : public HttpCouchGet {
 public:
    HttpBinaryPut(const char *ssid, const char *ssidPswd, 
		  const char *host, int port, const char *page,
		  const char *dbUser, const char *dbPswd, 
		  bool isSSL, HttpDataProvider *provider, const char *contentType)
      : HttpCouchGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
        m_provider(provider), m_contentType(contentType), m_writtenCnt(0), mRetryFlush(false)
    {
    }
    HttpBinaryPut(const Str &ssid, const Str &ssidPswd, 
		  const Str &host, int port, const char *page,
		  const Str &dbUser, const Str &dbPswd, 
		  bool isSSL, HttpDataProvider *provider, const char *contentType)
      : HttpCouchGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
        m_provider(provider), m_contentType(contentType), m_writtenCnt(0), mRetryFlush(false)
    {
    }
    HttpBinaryPut(const char *ssid, const char *ssidPswd, 
		  const IPAddress &hostip, int port, const char *page,
		  const char *dbUser, const char *dbPswd, 
		  bool isSSL, HttpDataProvider *provider, const char *contentType)
      : HttpCouchGet(ssid, ssidPswd, hostip, port, page, dbUser, dbPswd, isSSL),
        m_provider(provider), m_contentType(contentType), m_writtenCnt(0)
      {
      }
    virtual ~HttpBinaryPut() {}

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
    bool mRetryFlush;
};

#endif
