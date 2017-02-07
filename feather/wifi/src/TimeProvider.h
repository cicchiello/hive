#ifndef timeprovider_h
#define timeprovider_h

class TimeProvider {
 public:
    virtual void toString(unsigned long now, Str *str) const = 0;

    virtual bool haveTimestamp() const;
};

#endif
