#ifndef http_stream_test_h
#define http_stream_test_h

#include <http_putbase.t.h>

class HttpBinaryPut;
class ADCDataProvider;
class Str;

class HttpStreamTest : public HttpPutBaseTest {
  public:
    HttpStreamTest();
    HttpStreamTest(const char *dbUser, const char *dbPswd, 
		   const char *dbHost,
		   int dbPort,
		   bool isSSL);
    ~HttpStreamTest();
    
    bool loop();

    const char *testName() const {return "HttpStreamTest";}

    virtual bool createPutter(const CouchUtils::Doc &doc);
    
    const char *getDocid() const;
      
  private:
    HttpStreamTest(const HttpStreamTest&); // intentionally unimplemented

  protected:
    HttpBinaryPut *m_putter;
    ADCDataProvider *m_provider;
    unsigned long mNow, mTransferStart, mTransferTime;
};

#endif
