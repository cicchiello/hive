#ifndef sdcard_write_h
#define sdcard_write_h

#include <tests.h>


class SDCardWrite : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardWrite";}

    static bool makeFile(const char *filename);
};

#endif
