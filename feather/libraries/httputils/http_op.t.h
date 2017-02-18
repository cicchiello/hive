#ifndef http_op_test_h
#define http_op_test_h

#include <tests.h>

class Str;


class HttpOpTest : public Test{
 public:
    static const char *defaultDocid;   // common docid
    static const char *defaultDbName;     // common couchdb name

    static const char *defaultDbUser;
    static const char *defaultDbPswd;
    static const char *defaultDbHost;
    static const int defaultDbPort;

    static const char *sslDbUser;
    static const char *sslDbPswd;
    static const char *sslDbHost;
    static const int sslDbPort;

    const char *getDbUser() const;
    const char *getDbPswd() const;
    const char *getDbHost() const;
    int getDbPort() const {return mDbPort;}
    bool getIsSSL() const {return mIsSSL;}

 protected:
    HttpOpTest(const char *dbUser, const char *dbPswd, 
	       const char *dbHost,
	       int dbPort,
	       bool isSSL);
    ~HttpOpTest();
  
    bool setup();
    
    bool m_success;
    unsigned long m_timeToAct;

    Str *mDbUser, *mDbPswd, *mDbHost;
    int mDbPort;
    bool mIsSSL;
};

#endif
