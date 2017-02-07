#ifndef http_put_base_test_h
#define http_put_base_test_h

#include <http_op.t.h>

#include <couchutils.h>


class HttpPutBaseTest : public HttpOpTest {
 public:
  HttpPutBaseTest(const char *credentials,
		  const char *dbHost,
		  int dbPort,
		  bool isSSL);
  ~HttpPutBaseTest();

  bool loop();

  virtual bool createPutter(const CouchUtils::Doc &doc) = 0;

  virtual const char *getDocid() const = 0;
 
 protected:
  class HttpCouchGet *m_getter;
};

#endif
