#ifndef http_couchput_h
#define http_couchput_h

#include <http_couchget.h>


class HttpCouchPut : private HttpCouchGet {
 public:
   HttpCouchPut(const Str &ssid, const Str &ssidPswd, 
		const Str &host, int port, 
		CouchUtils::Doc *content, // passing ownership
		const Str &dbUser, const Str &dbPswd, bool isSSL, const char *page[])
     : HttpCouchGet(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL, page),
       m_content(content)
       {
       }
   ~HttpCouchPut();

   using HttpCouchGet::EventResult;
   
   using HttpCouchGet::getHeaderConsumer;
   using HttpCouchGet::getCouchConsumer;
   using HttpCouchGet::getDoc;
   using HttpCouchGet::haveDoc;
   using HttpCouchGet::processEventResult;
   using HttpCouchGet::isTimeout;
   using HttpCouchGet::isError;
   using HttpCouchGet::shutdownWifiOnDestruction;

   EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);
   

 protected:
   HttpCouchPut(const HttpCouchPut &); //intentionally unimplemented

   void sendPUT(Stream &s, int contentLength) const;
   void sendDoc(Stream &s, const CouchUtils::Doc &doc) const;
   
   bool testSuccess() const;

   const CouchUtils::Doc *m_content;
};


#endif


