#ifndef http_couchpost_h
#define http_couchpost_h

#include <http_couchget.h>


class HttpCouchPost : private HttpCouchGet {
 public:
   HttpCouchPost(const char *ssid, const char *ssidPswd, 
		 const char *host, int port, const char *page,
		 const CouchUtils::Doc &content,
		 const char *dbUser, const char *dbPswd, bool isSSL = false)
     : HttpCouchGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
       m_content(content)
       {
       }
   HttpCouchPost(const Str &ssid, const Str &ssidPswd, 
		 const Str &host, int port, const char *page,
		 const CouchUtils::Doc &content,
		 const Str &dbUser, const Str &dbPswd, bool isSSL = false)
     : HttpCouchGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
       m_content(content)
       {
       }
   HttpCouchPost(const char *ssid, const char *ssidPswd, 
		 const IPAddress &hostip, int port, const char *page,
		 const CouchUtils::Doc &content,
		 const char *dbUser, const char *dbPswd, bool isSSL = false)
     : HttpCouchGet(ssid, ssidPswd, hostip, port, page, dbUser, dbPswd, isSSL),
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

   EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);
   

 protected:
   HttpCouchPost(const HttpCouchPost &); //intentionally unimplemented

   void sendPOST(Stream &s, int contentLength) const;
   void sendDoc(Stream &s, const char *doc) const;
   
   bool testSuccess() const;

   const CouchUtils::Doc m_content;
};


#endif


