#ifndef hive_platform_h
#define hive_platform_h


class HivePlatform {
 public:
    static const char *getResetCause();
    
    static void startWDT();

    static void clearWDT();

    static void trace(const char *msg);

    static void error(const char *msg);
};


#endif
