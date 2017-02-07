#ifndef sdcard_reader_h
#define sdcard_reader_h

#include <tests.h>

class SDCardRead : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "SDCardRead";}

    static bool testFile(const char *filename);
};

#endif
