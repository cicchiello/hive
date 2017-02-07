#ifndef dac_sin_test_h
#define dac_sin_test_h

#include <tests.h>


class DAC_SinTest : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "DAC_SinTest";}
};

#endif
