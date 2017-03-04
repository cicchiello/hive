#ifndef http_couchget_h
#define http_couchget_h

#include <http_get.h>
#include <http_couchconsumer.h>
#include <couchutils.h>


class HttpCouchGet : public HttpGet {
 public:
   HttpCouchGet(const char *ssid, const char *ssidPswd, 
		const char *host, int port, const char *page,
		const char *dbUser, const char *dbPswd, 
		bool isSSL = false)
     : HttpGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
       m_consumer(getContext())
       {
	 init();
       }
  
   HttpCouchGet(const Str &ssid, const Str &ssidPswd, 
		const Str &host, int port, const Str &page,
		const Str &dbUser, const Str &dbPswd, 
		bool isSSL = false)
     : HttpGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
       m_consumer(getContext())
       {
	 init();
       }
  
   HttpCouchGet(const char *ssid, const char *ssidPswd, 
		const IPAddress &hostip, int port, const char *page,
		const char *dbUser, const char *dbPswd, 
		bool isSSL = false)
     : HttpGet(ssid, ssidPswd, hostip, port, page, dbUser, dbPswd, isSSL),
       m_consumer(getContext())
       {
	 init();
       }
  
   ~HttpCouchGet() {}
   
   virtual HttpHeaderConsumer &getHeaderConsumer() {return m_consumer;}
   virtual const HttpHeaderConsumer &getHeaderConsumer() const {return m_consumer;}

   HttpCouchConsumer &getCouchConsumer() {return m_consumer;}
   const HttpCouchConsumer &getCouchConsumer() const {return m_consumer;}
   
   bool haveDoc() const;
   const CouchUtils::Doc &getDoc() const;

   bool isError() const {return m_consumer.isError();}
   bool hasNotFound() const {return m_consumer.hasNotFound();}
   bool isTimeout() const {return m_consumer.isTimeout();}
   
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


