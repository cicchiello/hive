#ifndef actuator_h
#define actuator_h


class Actuator {
 public:
    Actuator(unsigned long now);
    ~Actuator() {}

    virtual bool isItTimeYet(unsigned long now);
    virtual void scheduleNextAction(unsigned long now);

    virtual void act() = 0;
    
    virtual bool isMyCommand(const char *response) const;
    virtual char * processCommand(const char *response);

 protected:
    unsigned long mNextActionTime;
};


#endif
