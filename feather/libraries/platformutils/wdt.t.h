#ifndef wdt_test_h
#define wdt_test_h

#include <tests.h>


class WDTTest : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "WDTTest";}
};

#endif
