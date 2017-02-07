#ifndef http_sslbinaryput_test_h
#define http_sslbinaryput_test_h

#include <http_binput.t.h>


class HttpSSLBinaryPutTest : public HttpBinaryPutTest {
  public:
    HttpSSLBinaryPutTest();
    ~HttpSSLBinaryPutTest();

    const char *testName() const {return "HttpSSLBinaryPutTest";}

    virtual bool createPutter(const CouchUtils::Doc &originalDoc);
};

#endif
