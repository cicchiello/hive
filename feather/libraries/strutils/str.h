#ifndef str_h
#define str_h

class Str {
 public:
  Str();
  Str(const char *s);
  Str(const Str &str);
  ~Str();

  Str &operator=(const Str &);
  Str &operator=(const char *);

  Str &append(const char *str);
  Str &append(const Str &str);
  Str &append(int i);
  Str &append(long l);
  Str &append(unsigned long l);
  Str &append(char c) {add(c);}
  
  void add(char c);
  void set(char c, int i);
  void clear();
  
  int capacity() const;
  int len() const;

  bool equals(const Str &other) const;
  
  void expand(int capacity);

  const char *c_str() const;

  static int sBytesConsumed;
  
 private:
  char *buf;
  int cap;
  bool deleted;
};

inline Str::Str() : buf(0), cap(0), deleted(false) {}
inline const char *Str::c_str() const {return buf;}
inline int Str::capacity() const {return cap;}
inline void Str::set(char c, int i) {buf[i] = c;}

#endif
