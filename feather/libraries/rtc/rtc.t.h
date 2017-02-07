#ifndef rtc_test_h
#define rtc_test_h

#include <http_get.t.h>

class RTCTest : public HttpGetTest {
 public:
    RTCTest() {}
    ~RTCTest();
  
    bool loop();

    const char *testName() const {return "RTCTest";}

 private:
    class RTCTestGetter *m_rtcGetter;
};

#endif
