#ifndef http_jsonpost_h
#define http_jsonpost_h

#include <http_jsonget.h>


class HttpJSONPost : private HttpJSONGet {
 public:
   HttpJSONPost(const Str &ssid, const Str &ssidPswd, 
		const Str &host, int port, 
		const CouchUtils::Doc &content,
		const Str &dbUser, const Str &dbPswd, bool isSSL, const char *page[])
     : HttpJSONGet(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL, page),
       m_content(content)
       {
       }
  
   ~HttpJSONPost() {}

   using HttpJSONGet::EventResult;
   
   using HttpJSONGet::getHeaderConsumer;
   using HttpJSONGet::getJSONConsumer;
   using HttpJSONGet::getDoc;
   using HttpJSONGet::haveDoc;
   using HttpJSONGet::processEventResult;
   using HttpJSONGet::isTimeout;
   using HttpJSONGet::isError;
   using HttpJSONGet::shutdownWifiOnDestruction;
   using HttpJSONGet::hasNotFound;
   using HttpJSONGet::getOpState;

   EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);
   

 protected:
   HttpJSONPost(const HttpJSONPost &); //intentionally unimplemented

   void sendPOST(Stream &s, int contentLength) const;
   void sendDoc(Stream &s, const CouchUtils::Doc &doc) const;
   
   bool testSuccess() const;

   CouchUtils::Doc m_content;
};


#endif


