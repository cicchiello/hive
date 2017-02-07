#ifndef sine_h
#define sine_h

class Sine {
 public:
  Sine(double freq, int sampleRate, int amplitude, int dcBias);
  ~Sine();

  unsigned short sineSample(long sampleNum);
  
 private:
  double mSamplesPerCycle;
  int mMinSin, mMaxSin, mSampleCnt, mCycleSample;
  unsigned short *mSamples;
};

#endif
