#ifndef http_couchget_h
#define http_couchget_h

#include <http_get.h>
#include <http_couchconsumer.h>
#include <couchutils.h>


class HttpCouchGet : public HttpGet {
 public:
   HttpCouchGet(const char *ssid, const char *ssidPswd, 
		const char *host, int port, const char *page,
		const char *credentials, bool isSSL = false);
   HttpCouchGet(const char *ssid, const char *ssidPswd, 
		const IPAddress &hostip, int port, const char *page,
		const char *credentials, bool isSSL = false);
   ~HttpCouchGet();
   
   virtual HttpHeaderConsumer &getHeaderConsumer() {return m_consumer;}
   virtual const HttpHeaderConsumer &getHeaderConsumer() const {return m_consumer;}

   bool haveDoc() const;
   const CouchUtils::Doc &getDoc() const;

 protected:
   HttpCouchGet(const HttpCouchGet &); //intentionally unimplemented

   bool testSuccess() const ;
   
   void init();
   
   void resetForRetry();

   HttpCouchConsumer m_consumer;
   CouchUtils::Doc m_doc;
   bool m_parsedDoc, m_haveDoc;
};

inline
const CouchUtils::Doc &HttpCouchGet::getDoc() const
{
    return m_doc;
}

#endif


