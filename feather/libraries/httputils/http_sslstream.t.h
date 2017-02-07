#ifndef http_sslstream_test_h
#define http_sslstream_test_h

#include <http_stream.t.h>

class HttpSSLStreamTest : public HttpStreamTest {
  public:
    HttpSSLStreamTest();
    ~HttpSSLStreamTest();
    
    const char *testName() const {return "HttpSSLStreamTest";}

    //    virtual bool createPutter(const CouchUtils::Doc &doc);
};

#endif
