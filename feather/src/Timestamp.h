#ifndef timestamp_h
#define timestamp_h

class Adafruit_BluefruitLE_SPI;
class Str;


class Timestamp {
 public:

    class QueueEntry {
    public:
        QueueEntry() {}
      
	void post(Adafruit_BluefruitLE_SPI &ble);
    };
  
    Timestamp() : mRequestedTimestamp(false), mHaveTimestamp(false) {}
    ~Timestamp() {}

    void toString(unsigned long now, Str *str);
    
    void enqueueRequest();

    void attemptPost(Adafruit_BluefruitLE_SPI &ble);
    
    char *processTimestampResponse(const char *response);

    bool isTimestampResponse(const char *response) const;
    
    bool haveRequestedTimestamp() const;
    bool haveTimestamp() const;

    unsigned long secondsAtMark() const;

 private:
    bool mRequestedTimestamp, mHaveTimestamp;
    unsigned long mTimestamp, mSecondsAtMark;
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
