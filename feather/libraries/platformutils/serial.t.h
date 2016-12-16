#ifndef serial_test_h
#define serial_test_h

#include <tests.h>


class SerialTest : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SerialTest";}
};

#endif
