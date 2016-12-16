#ifndef bank_test_h
#define bank_test_h

#include <tests.h>


class BankTest : public Test{
  public:
    bool setup();
    bool loop();

    const char *testName() const {return "BankTest";}
};

#endif
