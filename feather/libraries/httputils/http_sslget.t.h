#ifndef http_sslget_test_h
#define http_sslget_test_h

#include <http_get.t.h>

class HttpSSLGetTest : public HttpGetTest {
  public:
    HttpSSLGetTest();
    ~HttpSSLGetTest();
    
    const char *testName() const {return "HttpSSLGetTest";}
};

#endif
