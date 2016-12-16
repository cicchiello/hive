#ifndef couchutils_test_h
#define couchutils_test_h

#include <tests.h>


class CouchUtilsTest : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "CouchUtilsTest";}
};

#endif
