#ifndef http_get_test_h
#define http_get_test_h

#include <http_op.t.h>


class HttpGetTest : public HttpOpTest {
 public:
    HttpGetTest();
    HttpGetTest(const char *dbUser, const char *dbPswd,
		const char *dbHost,
		int dbPort,
		bool isSSL);
   ~HttpGetTest();
  
   bool loop();

   const char *testName() const {return "HttpGetTest";}

   virtual const char *expectedContent() const;
   
 protected:
   class HttpCouchGet *m_getter;
};


#endif
