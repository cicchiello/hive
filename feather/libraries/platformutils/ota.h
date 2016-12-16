#ifndef ota_h
#define ota_h

#include <Arduino.h>

class OTA {
 public:
    static const OTA &singleton();
    static OTA &nonConstSingleton();

    enum FlashBank {
      Bank0, Bank1,
      NumFlashBanks,
      UndefinedBank
    };
    FlashBank getThisFlashBank() const;
    FlashBank getWhichFlashBank(const void *) const;

 private:
    static OTA s_singleton;

    bool eraseRange(void *dst, uint32_t len);

    friend class BankTest;
};

inline
const OTA &OTA::singleton()
{
    return s_singleton;
}

inline
OTA &OTA::nonConstSingleton()
{
    return s_singleton;
}


#endif
