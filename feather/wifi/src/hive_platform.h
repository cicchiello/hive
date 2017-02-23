#ifndef hive_platform_h
#define hive_platform_h


class HivePlatform {
 public:
    static const HivePlatform *singleton();
    static HivePlatform *nonConstSingleton();
    
    const char *getResetCause() const;
    
    void startWDT();
    void clearWDT();

    void markWDT(const char *msg) const;

    static void trace(const char *msg);
    static void error(const char *msg);

    static const int SAMPLES_PER_SECOND_20K = 20000;
    void registerPulseGenConsumer_10K(class PulseGenConsumer *consumer);
    void registerPulseGenConsumer_20K(class PulseGenConsumer *consumer);
    void pulseGen_20K_init();

 private:
    HivePlatform();
};


#endif
