#ifndef strbuf_h
#define strbuf_h

class Str;

class StrBuf {
 public:
  StrBuf();
  StrBuf(const char *s);
  StrBuf(const StrBuf &strbuf);
  ~StrBuf();

  StrBuf &append(const char *str);
  StrBuf &append(int i);
  StrBuf &append(long l);
  StrBuf &append(unsigned long l);
  StrBuf &append(char c) {add(c); return *this;}
  
  void add(char c);
  void set(char c, int i);
  StrBuf tolower() const;
  
  int capacity() const;
  int len() const;

  const char *c_str() const;

  StrBuf &operator=(const char *o);
  StrBuf &operator=(const StrBuf &o);
  StrBuf &operator+=(char c) {return append(c);}
  
  void expand(int capacity);
  
 private:
  char *buf;
  unsigned short cap;
  bool deleted;
};

inline StrBuf::StrBuf() : buf(0), cap(0), deleted(false) {}
inline const char *StrBuf::c_str() const {return buf;}
inline int StrBuf::capacity() const {return cap;}
inline void StrBuf::set(char c, int i) {buf[i] = c;}


#endif
