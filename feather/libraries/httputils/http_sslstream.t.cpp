#include <http_sslstream.t.h>

//#define NDEBUG
#include <strutils.h>

#include <Trace.h>

HttpSSLStreamTest::HttpSSLStreamTest()
  : HttpStreamTest(sslDbCredentials, sslDbHost, sslDbPort, true)
{
}

HttpSSLStreamTest::~HttpSSLStreamTest() {}

