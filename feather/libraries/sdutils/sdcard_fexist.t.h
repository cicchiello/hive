#ifndef sdcard_fexist_h
#define sdcard_fexist_h

#include <tests.h>

class SDCardFExist : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardFExist";}
};

#endif
