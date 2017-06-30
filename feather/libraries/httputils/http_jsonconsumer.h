#ifndef http_jsonconsumer_h
#define http_jsonconsumer_h

#include <http_headerconsumer.h>
#include <couchdoc_consumer.h>
#include <jparse.h>


class HttpJSONConsumer : public HttpHeaderConsumer {
 public:
   HttpJSONConsumer(const WifiUtils::Context &ctxt);
   ~HttpJSONConsumer();

   bool hasDoc() const {return mConsumer.haveDoc();}
   const CouchUtils::Doc &getDoc() const {return mDoc;}
  
   bool consume(unsigned long now);

   const StrBuf &getETag() const {return mEtag;}
   const StrBuf &getTimestamp() const {return mTimestamp;}

   void reset();

 protected:
   void parseHeaderLine(const StrBuf &line);
  
 private:
   void init();

   CouchUtils::Doc mDoc;
   CouchDocConsumer mConsumer;
   JParse mParser;

   StrBuf mEtag, mTimestamp;
};

#endif
