#ifndef sdcard_rawwrite_h
#define sdcard_rawwrite_h

#include <SdFat.h>

#include <tests.h>


class SDCardRawWrite : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardRawWrite";}
};

#endif
