#ifndef http_fileput_test_h
#define http_fileput_test_h

#include <http_putbase.t.h>

class HttpFilePut;
class Str;

class HttpFilePutTest : public HttpPutBaseTest {
  public:
    HttpFilePutTest();
    HttpFilePutTest(const char *dbUser, const char *dbPswd,
		    const char *dbHost,
		    int dbPort,
		    bool isSSL);
    ~HttpFilePutTest();
    
    bool loop();

    const char *testName() const {return "HttpFilePutTest";}

    virtual bool createPutter(const CouchUtils::Doc &doc);
    
    const char *getDocid() const;
      
  private:
    HttpFilePutTest(const HttpFilePutTest&); // intentionally unimplemented

  protected:
    HttpFilePut *m_putter;
    unsigned long mNow, mTransferStart, mTransferTime;
};

#endif
