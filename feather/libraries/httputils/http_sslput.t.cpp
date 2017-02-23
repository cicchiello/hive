#include <http_sslput.t.h>

#define NDEBUG
#include <strutils.h>


//
// Most code taken from Adafruit_WINC1500/examples/WifiWebClient.ino
//


HttpSSLPutTest::HttpSSLPutTest()
  : HttpPutTest(sslDbUser, sslDbPswd, sslDbHost, sslDbPort, true)
{
}

HttpSSLPutTest::~HttpSSLPutTest() {}


