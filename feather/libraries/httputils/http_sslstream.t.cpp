#include <http_sslstream.t.h>

#include <Arduino.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

HttpSSLStreamTest::HttpSSLStreamTest()
  : HttpStreamTest(sslDbUser, sslDbPswd, sslDbHost, sslDbPort, true)
{
}

HttpSSLStreamTest::~HttpSSLStreamTest() {}

