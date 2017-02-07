#ifndef http_sslfileput_test_h
#define http_sslfileput_test_h

#include <http_fileput.t.h>

class HttpSSLFilePutTest : public HttpFilePutTest {
  public:
    HttpSSLFilePutTest();
    
    const char *testName() const {return "HttpSSLFilePutTest";}

  private:
    HttpSSLFilePutTest(const HttpSSLFilePutTest&); // intentionally unimplemented
};

#endif
