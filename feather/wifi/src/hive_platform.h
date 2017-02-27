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

    static void stackDump();
    
    static void trace(const char *msg);
    static void error(const char *msg);

    static const int SAMPLES_PER_SECOND_22K = 22050;
    void registerPulseGenConsumer_11K(class PulseGenConsumer *consumer);
    void registerPulseGenConsumer_22K(class PulseGenConsumer *consumer);

    void unregisterPulseGenConsumer_11K(class PulseGenConsumer *consumer);
    void unregisterPulseGenConsumer_22K(class PulseGenConsumer *consumer);
    
    void pulseGen_22K_init();

 private:
    HivePlatform();
};


#endif
