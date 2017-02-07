#ifndef tests_h
#define tests_h

class Test {
  public:
    static char ssid[];     //  your network SSID (name)
    static char pass[];  // your network password
 
    Test() : m_didIt(false) {}
    virtual ~Test() {}

    virtual bool setup() {return true;};
    virtual bool loop() {return true;};

    virtual bool isDone() const;
    virtual const char *testName() const = 0;

  protected: 
    bool m_didIt;
};

class Tests {
  public:
    enum AvailableTests {
      RTC_TEST,
      SDCARD_FNEXIST,
      SDCARD_LS,
      SDCARD_WRITE,
      SDCARD_RAWWRITE,
      SDCARD_FEXIST,
      SDCARD_READ,
      SDCARD_RM,
      WIFI_VERSION,
      WIFI_SCAN,
      WIFI_CONNECT,
      SERIAL_TEST,
      ADC_SAMPLE,
      LISTEN_TEST,
      DAC_SIN_TEST,
      COUCHUTILS_TEST,
      HTTP_GET_TEST,
      HTTP_SSLGET_TEST,
      HTTP_PUT_TEST,
      HTTP_SSLPUT_TEST,
      HTTP_BINARYPUT_TEST,
      HTTP_FILEPUT_TEST,
      HTTP_SSLFILEPUT_TEST,
      HTTP_SSLBINARYPUT_TEST,
      WDT_TEST,
      PITCHER_TEST,
      CATCHER_TEST,
      endOfAvailableTests
    };
  
    Tests(int selectedTests[]);
  
    Tests(); // full test suite

    void setup();

    void loop();
    
  private:
    void init(int selectedTests[]);
};

#endif
