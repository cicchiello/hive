#ifndef json_consumer_h
#define json_consumer_h

#include <jparse.h>


class JSONConsumer {
 public:
  virtual void openDoc() = 0;
  virtual void openArr() = 0;
  virtual void closeDoc() = 0;
  virtual void closeArr() = 0;
  virtual void nv_name(const char *identifier) = 0;
  virtual void nv_valueString(const char *value) = 0;
  virtual void nv_valueInteger(int value) = 0;
  virtual void arr_string(const char *element) = 0;
  virtual void arr_integer(int element) = 0;
};

#endif
