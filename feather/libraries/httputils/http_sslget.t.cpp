#include <http_sslget.t.h>


#define NDEBUG
#include <strutils.h>

//the following linux command line will report the encoded credentials (among other things):
//> curl -v -X GET https://witerearchemetoodgmespec:117f77d4f8efae6173bedb172dc3dc174615a7d6@jfcenterprises.cloudant.com/test-persistent-enum/feather-get-test


HttpSSLGetTest::HttpSSLGetTest()
  : HttpGetTest(sslDbCredentials, sslDbHost, sslDbPort, true)
{
}

HttpSSLGetTest::~HttpSSLGetTest()
{
}

