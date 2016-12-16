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
      WIFI_DISCONNECT,
      ADC_SAMPLE,
      DAC_SIN_TEST,
      LISTEN_TEST,
      SERIAL_TEST,
      PULSE_TEST,
      COUCHUTILS_TEST,
      HTTP_GET_TEST,
      HTTP_SSLGET_TEST,
      HTTP_PUT_TEST,
      HTTP_SSLPUT_TEST,
      HTTP_BINARYPUT_TEST,
      HTTP_BINARYGET_TEST,
      WDT_TEST,
      BANK_TEST,
      PITCHER_TEST,
      CATCHER_TEST,
      PUBNUB_CERTS,
      LIFI_TEST,
      LIFI_TX_TEST,
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
