#ifndef pulse_test_h
#define pulse_test_h

#include <tests.h>


class PulseTest : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "PulseTest";}
};

#endif
