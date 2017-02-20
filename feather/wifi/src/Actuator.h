#ifndef actuator_h
#define actuator_h

class Str;
class Mutex;


class Actuator {
 public:
    static const int MAX_ACTUATORS = 20;
    
 private:
    static int sNumActiveActuators;
    static Actuator *sActiveActuators[MAX_ACTUATORS];
    
 public:
    // Manage a list of "active" actuators
    static void activate(Actuator *actuator);
    static void deactivate(int i);
    static int getNumActiveActuators() {return sNumActiveActuators;}
    static Actuator *getActiveActuator(int i) {return sActiveActuators[i];}
    
    Actuator(const char *name, unsigned long now);
    ~Actuator();

    virtual bool isMyMsg(const char *msg) const = 0;
    
    virtual bool loop(unsigned long now, Mutex *wifi) = 0;

    virtual const char *getName() const;
    
 protected:
    virtual const char *className() const = 0;
    
    Str TAG(const char *memberfunc, const char *msg) const;

    Str *mName;
    unsigned long mNextActionTime;
    bool mIsActive;
};


#endif
