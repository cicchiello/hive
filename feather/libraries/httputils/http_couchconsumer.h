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

   void reset();

 private:
   void init() {}

   void cleanChunkedResult(const char *terminationMarker);
};

#endif
