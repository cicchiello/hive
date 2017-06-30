#ifndef http_couchconsumer_h
#define http_couchconsumer_h

#include <http_headerconsumer.h>
#include <couchutils.h>

class HttpCouchConsumer : public HttpHeaderConsumer {
 public:
   HttpCouchConsumer(const WifiUtils::Context &ctxt);
   ~HttpCouchConsumer();

   const char *parseDoc(CouchUtils::Doc *doc) const;
  
   bool consume(unsigned long now);

   const StrBuf &getETag() const {return mEtag;}
   const StrBuf &getTimestamp() const {return mTimestamp;}

   const StrBuf &getContent() const {return m_content;}
   
   void reset();

 protected:
   void parseHeaderLine(const StrBuf &line);
  
 private:
   void init();

   void cleanChunkedResultInPlace(const char *terminationMarker);
   
   StrBuf mEtag, mTimestamp;
   StrBuf m_content;
};

#endif
