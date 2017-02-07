#ifndef http_fileput_h
#define http_fileput_h

#include <http_couchget.h>


class HttpFilePut : private HttpCouchGet {
 public:
    HttpFilePut(const char *ssid, const char *ssidPswd, 
		const char *host, int port, const char *page, const char *credentials,
		bool isSSL, const char *filename, const char *contentType);
    HttpFilePut(const char *ssid, const char *ssidPswd, 
		const IPAddress &hostip, int port, const char *page, const char *credentials,
		bool isSSL, const char *filename, const char *contentType);
    virtual ~HttpFilePut();

    virtual int getContentLength() const;
    
    virtual EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);

    using HttpCouchGet::EventResult;
   
    using HttpCouchGet::getHeaderConsumer;
    using HttpCouchGet::getDoc;
    using HttpCouchGet::haveDoc;
    using HttpCouchGet::processEventResult;
    using HttpCouchGet::getFinalResult;

 protected:
    HttpFilePut(const HttpFilePut &); //intentionally unimplemented

    void init();
    
    void resetForRetry();
    
    void sendPUT(Stream &s, int contentLength) const;
   
    const Str m_filename, m_contentType;

    class SdFat *m_sd;
    class SdFile *m_f;
    int m_writtenCnt;
};

#endif
