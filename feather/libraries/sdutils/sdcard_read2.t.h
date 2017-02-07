#ifndef sdcard_reader2_h
#define sdcard_reader2_h

#include <tests.h>

class SDCardRead2 : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardRead2";}
};

#endif
