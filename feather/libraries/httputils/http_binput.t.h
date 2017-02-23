#ifndef http_binput_test_h
#define http_binput_test_h

#include <http_putbase.t.h>

class HttpBinaryPut;
class HttpDataProvider;
class Str;

class HttpBinaryPutTest : public HttpPutBaseTest {
  public:
    HttpBinaryPutTest();
    HttpBinaryPutTest(const char *dbUser, const char *dbPswd,
		      const char *dbHost,
		      int dbPort,
		      bool isSSL);
    ~HttpBinaryPutTest();
    
    bool loop();

    const char *testName() const {return "HttpBinaryPutTest";}

    virtual bool createPutter(const CouchUtils::Doc &doc);
    
    const char *getDocid() const;
      
  private:
    HttpBinaryPutTest(const HttpBinaryPutTest&); // intentionally unimplemented

  protected:
    HttpBinaryPut *m_putter;
    HttpDataProvider *m_provider;
    unsigned long mNow, mTransferStart, mTransferTime;
};

#endif
