#ifndef sdcard_fnexist_h
#define sdcard_fnexist_h

#include <tests.h>

class SDCardFNExist : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardFNExist";}
};

#endif
