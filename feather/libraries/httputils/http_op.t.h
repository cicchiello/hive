#ifndef http_op_test_h
#define http_op_test_h

#include <tests.h>

class Str;


class HttpOpTest : public Test{
 public:
    static const char *defaultDocid;   // common docid
    static const char *defaultDbName;     // common couchdb name

    static const char *defaultDbCredentials;
    static const char *defaultDbHost;
    static const int defaultDbPort;

    static const char *sslDbCredentials;
    static const char *sslDbHost;
    static const int sslDbPort;
    
    const char *getCredentials() const;
    const char *getDbHost() const;
    int getDbPort() const {return mDbPort;}
    bool getIsSSL() const {return mIsSSL;}

 protected:
    HttpOpTest(const char *credentials,
	       const char *dbHost,
	       int dbPort,
	       bool isSSL);
    ~HttpOpTest();
  
    bool setup();
    
    bool m_success;
    unsigned long m_timeToAct;

    Str *mCredentials, *mDbHost;
    int mDbPort;
    bool mIsSSL;
};

#endif
