#ifndef http_rawconsumer_h
#define http_rawconsumer_h

#include <http_headerconsumer.h>
#include <couchutils.h>

class HttpRawConsumer : public HttpHeaderConsumer {
public:
  HttpRawConsumer(WifiUtils::Context &ctxt) : HttpHeaderConsumer(ctxt) {}
  ~HttpRawConsumer() {}

  bool consume(unsigned long now);

private:
  StrBuf m_content;
};

#endif
