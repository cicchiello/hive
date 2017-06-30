#ifndef http_jsonget_h
#define http_jsonget_h

#include <http_get.h>
#include <http_jsonconsumer.h>


class HttpJSONGet : public HttpGet {
 public:
   HttpJSONGet(const Str &ssid, const Str &ssidPswd, 
	       const Str &host, int port, 
	       const Str &dbUser, const Str &dbPswd, 
	       bool isSSL, const char *urlPieces[]);
  
   ~HttpJSONGet() {}
   
   virtual HttpHeaderConsumer &getHeaderConsumer() {return mConsumer;}
   virtual const HttpHeaderConsumer &getHeaderConsumer() const {return mConsumer;}

   HttpJSONConsumer &getJSONConsumer() {return mConsumer;}
   const HttpJSONConsumer &getJSONConsumer() const {return mConsumer;}
   
   bool haveDoc() const;
   const CouchUtils::Doc &getDoc() const;

   bool isError() const {return mConsumer.isError();}
   bool hasNotFound() const {return mConsumer.hasNotFound();}
   bool isTimeout() const {return mConsumer.isTimeout();}
   
   const StrBuf &getETag() const;
   bool getTimestamp(StrBuf *date) const;
   bool hasTimestamp() const;
   
 protected:
   HttpJSONGet(const HttpJSONGet &); //intentionally unimplemented

   bool testSuccess() const ;
   
   void init();
   
   void resetForRetry();

   HttpJSONConsumer mConsumer;
};


inline
const CouchUtils::Doc &HttpJSONGet::getDoc() const
{
  return mConsumer.getDoc();
}

#endif


