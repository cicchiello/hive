#include <sine.h>

#include <math.h>

#define PI  3.14159265359

Sine::Sine(double freq, int sampleRate, int amplitude, int dcBias)
{
    mSamplesPerCycle = sampleRate/freq; // samples/s / cycles/s == samples/cycle
    mMinSin = dcBias-amplitude/2;
    mMaxSin = dcBias+amplitude/2;
    mSampleCnt = mCycleSample = 0;

    mSamples = new unsigned short[(int)(mSamplesPerCycle+10)];
    
    for (int i = 0; i < (int) mSamplesPerCycle; i++) {
        float rads = 2.0*PI*((float)i)/mSamplesPerCycle;
	double d = sin(rads) + 1.0;

	unsigned short s = (unsigned short) ((mMaxSin-mMinSin) * (d/2.0)) + mMinSin;
	mSamples[i] = s;
	//P("Sample["); P(i); P("] = "); PL(s);
    }
}


Sine::~Sine()
{
    delete [] mSamples;
}


unsigned short Sine::sineSample(long sampleNum)
{
    static int s_cycleSample = 0;

    if (mSampleCnt != sampleNum) {
        //P("Adjusting sample counter; was "); P(s_sampleCnt); P(" should be "); PL(sampleNum);
        int numCycles = (int) (((double)sampleNum) / mSamplesPerCycle);
	int cycleSample = sampleNum - ((double) numCycles)*mSamplesPerCycle;
	mSampleCnt = sampleNum;
	mCycleSample = cycleSample;
    }
    unsigned short s = mSamples[mCycleSample++];
    mSampleCnt++;
    if (mCycleSample >= (int) mSamplesPerCycle)
        mCycleSample = 0;
    return s;
}
