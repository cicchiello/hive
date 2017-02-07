#ifndef http_sslput_test_h
#define http_sslput_test_h

#include <http_put.t.h>


class HttpSSLPutTest : public HttpPutTest {
  public:
    HttpSSLPutTest();
    ~HttpSSLPutTest();
    
    const char *testName() const {return "HttpSSLPutTest";}
};

#endif
