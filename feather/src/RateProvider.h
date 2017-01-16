#ifndef rateprovider_h
#define rateprovider_h

class RateProvider {
 public:
  virtual int secondsBetweenSamples() const = 0;
};

#endif
