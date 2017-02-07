#ifndef sdutils_h
#define sdutils_h

class SDUtils {
  public:
    static const int SPI_CS;  // SPI chip select pin
  
    enum ReadlineStatus {
        ReadSuccess, ReadEOF, ReadBufOverflow
    };
    
    static ReadlineStatus readline(class SdFile *f, char *buf, int bufsz);

    static bool initSd(class SdFat &sd);
};
#endif
