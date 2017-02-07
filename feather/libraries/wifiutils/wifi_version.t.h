#ifndef wifi_version_h
#define wifi_version_h

#include <tests.h>


class WifiVersion : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "WifiVersion";}
};

#endif
