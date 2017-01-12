#ifndef actuator_h
#define actuator_h

class Str;

class Actuator {
 public:
    Actuator(const char *name, unsigned long now);
    ~Actuator();

    virtual bool isItTimeYet(unsigned long now);
    virtual void scheduleNextAction(unsigned long now);

    virtual void act(class Adafruit_BluefruitLE_SPI &ble) = 0;
    
    virtual bool isMyCommand(const char *response) const = 0;
    virtual const char * processCommand(const char *response);

    virtual const char *getName() const;
    
 protected:
    virtual const char *className() const = 0;
    
    Str TAG(const char *memberfunc, const char *msg) const;

    Str *mName;
    unsigned long mNextActionTime;
};


#endif
