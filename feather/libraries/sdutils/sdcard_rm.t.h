#ifndef sdcard_rm_h
#define sdcard_rm_h

#include <tests.h>

class SDCardRm : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardRm";}
};

#endif
