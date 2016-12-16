#include <ota.h>

/* STATIC */
OTA OTA::s_singleton;


extern int main();

static const uint16_t PageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };


OTA::FlashBank OTA::getWhichFlashBank(const void *addr) const
{
    unsigned long laddr = (unsigned long) addr;
    if ((0x2000 <= laddr) && (laddr < 0x20000)) {
        return OTA::Bank0;
    } else if ((0x22000 <= laddr) && (laddr < 0x40000)) {
        return OTA::Bank1;
    } else {
        return OTA::UndefinedBank;
    }
}


OTA::FlashBank OTA::getThisFlashBank() const
{
    return getWhichFlashBank((const void *)&main);
}


bool OTA::eraseRange(void *dst, uint32_t len)
{
    // first make sure the erase range is entirely within one bank, and make sure that
    // bank isn't the currently active bank (the one we're running from!)
    OTA::FlashBank b0 = getWhichFlashBank(dst);
    OTA::FlashBank b1 = getWhichFlashBank(dst+len-1);
    if ((b0 != OTA::UndefinedBank) && (b0 != getThisFlashBank()) && (b0 == b1)) {
      
        // Note: the flash memory is erased in ROWS, that is in block of 4 pages.
        //       Even if the starting address is the last byte of a ROW the entire
        //       ROW is erased anyway.

      uint32_t dst_addr = (uint32_t) dst;
      uint32_t end_addr = (uint32_t) (dst+len-1);
      uint16_t PAGE_SIZE = PageSizes[NVMCTRL->PARAM.bit.PSZ];
      while (dst_addr < end_addr) {
          // Execute "ER" Erase Row
          NVMCTRL->ADDR.reg = dst_addr / 2;
          NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
          while (NVMCTRL->INTFLAG.bit.READY == 0)
            ;
          dst_addr += PAGE_SIZE * 4; // Skip to next ROW
      }
    }
}

