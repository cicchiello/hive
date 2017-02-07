#ifndef actuator_h
#define actuator_h

class Str;
class Mutex;


class Actuator {
 public:
    Actuator(const char *name, unsigned long now);
    ~Actuator();

    virtual bool isItTimeYet(unsigned long now) const;
    
    virtual bool loop(unsigned long now, Mutex *wifi) = 0;

    virtual const char *getName() const;
    
 protected:
    virtual const char *className() const = 0;
    
    Str TAG(const char *memberfunc, const char *msg) const;

    Str *mName;
    unsigned long mNextActionTime;
};


#endif
