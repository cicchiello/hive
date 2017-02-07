#ifndef listener_wav_h
#define listener_wav_h

class ListenerWavCreator {
public:
    static const unsigned char WAV_RESOLUTION = 16; // extend to 16bit resolution in wav file
    
    virtual ~ListenerWavCreator();
  
    virtual bool writeChunk() = 0;

    virtual const char *getErrmsg() const = 0;

    bool hasError() const {return getErrmsg();}
};

#endif
