#include <http_op.t.h>

#include <wifiutils.h>
#include <strutils.h>

#include <MyWiFi.h>

#include <str.h>


/* STATIC */
const char *HttpOpTest::defaultDocid = "feather-get-test";


// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the host:
//char host[] = "joes-mac-mini";    // domain name for test page (using DNS)
//const IPAddress HttpOpTest::defaultDbHost(192,168,2,85);  // numeric IP for test page (no DNS)
const char *HttpOpTest::defaultDbHost = "70.15.56.138";
//const char *HttpOpTest::defaultDbHost = "192.168.1.85";
//const IPAddress HttpOpTest::defaultDbHost(75,97,20,58);  // numeric IP for test page (no DNS)
const int HttpOpTest::defaultDbPort = 5984;
const char *HttpOpTest::defaultDbName = "test-persistent-enum";
const char *HttpOpTest::defaultDbUser = NULL;
const char *HttpOpTest::defaultDbPswd = NULL;


// SSL Stuff
const char *HttpOpTest::sslDbHost = "jfcenterprises.cloudant.com";    // domain name for test page (using DNS)
//const char *HttpOpTest::sslDbHost = "184.173.163.134";    // domain name for test page (using DNS)
const int HttpOpTest::sslDbPort = 443; // ssl port

// encoding of: witerearchemetoodgmespec:117f77d4f8efae6173bedb172dc3dc174615a7d6
//const char *HttpOpTest::sslDbCredentials = "d2l0ZXJlYXJjaGVtZXRvb2RnbWVzcGVjOjExN2Y3N2Q0ZjhlZmFlNjE3M2JlZGIxNzJkYzNkYzE3NDYxNWE3ZDY=";

const char *HttpOpTest::sslDbUser = "afteptsecumbehisomorther";
const char *HttpOpTest::sslDbPswd = "e4f286be1eef534f1cddd6240ed0133b968b1c9a";



HttpOpTest::HttpOpTest(const char *dbUser, const char *dbPswd, 
		       const char *dbHost,
		       int dbPort,
		       bool isSSL)
  : mDbUser(new Str(dbUser)), mDbPswd(new Str(dbPswd)), mDbHost(new Str(dbHost)),
    mDbPort(dbPort), mIsSSL(isSSL), m_timeToAct(500l), m_success(true)
{
}


HttpOpTest::~HttpOpTest()
{
  delete mDbUser;
  delete mDbPswd;
  delete mDbHost;
}


bool HttpOpTest::setup() {
    return m_success;
}

const char *HttpOpTest::getDbUser() const {return mDbUser->c_str();}
const char *HttpOpTest::getDbPswd() const {return mDbPswd->c_str();}

const char *HttpOpTest::getDbHost() const {return mDbHost->c_str();}
