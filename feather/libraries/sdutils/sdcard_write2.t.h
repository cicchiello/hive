#ifndef sdcard_write2_h
#define sdcard_write2_h

#include <tests.h>


class SDCardWrite2 : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardWrite2";}

    static bool makeFile(const char *filename);
};

#endif
