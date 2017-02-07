#ifndef wifi_scan_h
#define wifi_scan_h

#include <tests.h>


class WifiScan : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "WifiScan";}
};

#endif
