#ifndef listen_test_h
#define listen_test_h

#include <tests.h>

class ListenTest : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "ListenTest";}
};

#endif
