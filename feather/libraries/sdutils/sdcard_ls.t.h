#ifndef sdcard_ls_h
#define sdcard_ls_h

#include <tests.h>


class SDCardLS : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardLS";}
};

#endif
