#ifndef http_binaryput_h
#define http_binaryput_h

#include <http_couchget.h>

class HttpDataProvider;

class HttpBinaryPut : public HttpCouchGet {
 public:
    HttpBinaryPut(const Str &ssid, const Str &ssidPswd, 
		  const Str &host, int port, 
		  const Str &dbUser, const Str &dbPswd, 
		  bool isSSL, HttpDataProvider *provider, const Str &contentType,
		  const char *urlPieces[])
      : HttpCouchGet(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL, urlPieces),
        m_provider(provider), m_contentType(contentType), m_writtenCnt(0), mRetryFlush(false)
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
