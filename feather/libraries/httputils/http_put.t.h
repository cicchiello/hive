#ifndef http_put_test_h
#define http_put_test_h

#include <http_putbase.t.h>

class HttpCouchPut;
class HttpCouchGet;

#include <couchutils.h>

class HttpPutTest : public HttpPutBaseTest {
  public:
    HttpPutTest();
    HttpPutTest(const char *dbUser, const char *dbPswd,
		const char *dbHost,
		int dbPort,
		bool isSSL);
    ~HttpPutTest();

    bool loop();

    const char *testName() const {return "HttpPutTest";}

    bool createPutter(const CouchUtils::Doc &doc);
    
    const char *getDocid() const;
    
  protected:
    HttpPutTest(const HttpPutTest&); // intentionally unimplemented
    
    bool createUpdateDoc(const CouchUtils::Doc &originalDoc, CouchUtils::Doc *updatedDoc);

    HttpCouchPut *m_putter;
};


#endif
