#ifndef rtcconversions_h
#define rtcconversions_h

typedef unsigned long int Timestamp_t;

class RTCConversions {
 public:
  // returns true on success; all other cases are likely parsing problems
  static bool cvtToTimestamp(const char *readable, Timestamp_t *result);
};

#endif
