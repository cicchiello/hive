#ifndef wifi_connect_h
#define wifi_connect_h

#include <tests.h>

class WifiConnect : public Test{
  public:
    bool loop();

    const char *testName() const {return "WifiConnect";}
};

#endif
