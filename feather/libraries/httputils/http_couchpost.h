#ifndef http_couchpost_h
#define http_couchpost_h

#include <http_couchget.h>


class HttpCouchPost : private HttpCouchGet {
 public:
   HttpCouchPost(const Str &ssid, const Str &ssidPswd, 
		 const Str &host, int port, 
		 const CouchUtils::Doc &content,
		 const Str &dbUser, const Str &dbPswd, bool isSSL, const char *page[])
     : HttpCouchGet(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL, page),
       m_content(content)
       {
       }
  
   ~HttpCouchPost() {}

   using HttpCouchGet::EventResult;
   
   using HttpCouchGet::getHeaderConsumer;
   using HttpCouchGet::getCouchConsumer;
   using HttpCouchGet::getDoc;
   using HttpCouchGet::haveDoc;
   using HttpCouchGet::processEventResult;
   using HttpCouchGet::isTimeout;
   using HttpCouchGet::isError;
   using HttpCouchGet::shutdownWifiOnDestruction;
   using HttpCouchGet::hasNotFound;
   using HttpCouchGet::getOpState;

   EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);
   

 protected:
   HttpCouchPost(const HttpCouchPost &); //intentionally unimplemented

   void sendPOST(Stream &s, int contentLength) const;
   void sendDoc(Stream &s, const CouchUtils::Doc &doc) const;
   
   bool testSuccess() const;

   CouchUtils::Doc m_content;
};


#endif


