#include <http_sslfileput.t.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

HttpSSLFilePutTest::HttpSSLFilePutTest()
  : HttpFilePutTest(sslDbUser, sslDbPswd, sslDbHost, sslDbPort, true)
{
}

