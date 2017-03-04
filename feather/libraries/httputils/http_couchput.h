#ifndef http_couchput_h
#define http_couchput_h

#include <http_couchget.h>


class HttpCouchPut : private HttpCouchGet {
 public:
   HttpCouchPut(const char *ssid, const char *ssidPswd, 
		const char *host, int port, const char *page,
		CouchUtils::Doc *content, // passing ownership
		const char *dbUser, const char *dbPswd, bool isSSL = false)
     : HttpCouchGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
       m_content(content)
       {
       }
   HttpCouchPut(const Str &ssid, const Str &ssidPswd, 
		const Str &host, int port, const char *page,
		CouchUtils::Doc *content, // passing ownership
		const Str &dbUser, const Str &dbPswd, bool isSSL = false)
     : HttpCouchGet(ssid, ssidPswd, host, port, page, dbUser, dbPswd, isSSL),
       m_content(content)
       {
       }
   HttpCouchPut(const char *ssid, const char *ssidPswd, 
		const IPAddress &hostip, int port, const char *page,
		CouchUtils::Doc *content, // passing ownership
		const char *dbUser, const char *dbPswd, bool isSSL = false)
     : HttpCouchGet(ssid, ssidPswd, hostip, port, page, dbUser, dbPswd, isSSL),
       m_content(content)
       {
       }
   ~HttpCouchPut();

   using HttpCouchGet::EventResult;
   
   using HttpCouchGet::getHeaderConsumer;
   using HttpCouchGet::getDoc;
   using HttpCouchGet::haveDoc;
   using HttpCouchGet::processEventResult;

   EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);
   

 protected:
   HttpCouchPut(const HttpCouchPut &); //intentionally unimplemented

   void sendPUT(Stream &s, int contentLength) const;
   void sendDoc(Stream &s, const char *doc) const;
   
   bool testSuccess() const;

   const CouchUtils::Doc *m_content;
};


#endif


