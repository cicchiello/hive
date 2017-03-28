#ifndef timeprovider_h
#define timeprovider_h

class TimeProvider {
 public:
    virtual void toString(unsigned long now, Str *str) const = 0;

    virtual unsigned long getSecondsSinceEpoch(unsigned long now) const = 0;
};

#endif
