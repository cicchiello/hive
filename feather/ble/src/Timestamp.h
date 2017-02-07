#ifndef timestamp_h
#define timestamp_h

class Adafruit_BluefruitLE_SPI;
class Str;


class Timestamp {
 public:

    class QueueEntry {
    private:
        Timestamp *mParent;
	
    public:
        QueueEntry(Timestamp *parent) {mParent = parent;}

	Timestamp *getTimestamp() {return mParent;}
	
	const char *getName() const {return "timestamp";}
    
	void post(const char *sensorName, Adafruit_BluefruitLE_SPI &ble);
    };
  
    Timestamp(const char *resetCause, const char *versionId);
    ~Timestamp();

    const char *getName() const {return "timestamp";}
    // since there can be only one

    const char *getResetCause() const;

    const char *getVersionId() const;
    
    void toString(unsigned long now, Str *str);
    
    void enqueueRequest();

    void attemptPost(Adafruit_BluefruitLE_SPI &ble);
    
    void processTimestampResponse(Str *response);

    bool isTimestampResponse(const Str &response) const;
    
    bool haveRequestedTimestamp() const;
    bool haveTimestamp() const;

    unsigned long secondsAtMark() const;

 private:
    bool mRequestedTimestamp, mHaveTimestamp;
    unsigned long mTimestamp, mSecondsAtMark;
    Str *mRCause, *mVersionId;
};


inline
bool Timestamp::haveRequestedTimestamp() const
{
    return mRequestedTimestamp;
}

inline
bool Timestamp::haveTimestamp() const
{
    return mHaveTimestamp;
}

inline
unsigned long Timestamp::secondsAtMark() const
{
    return mSecondsAtMark;
}


#endif
