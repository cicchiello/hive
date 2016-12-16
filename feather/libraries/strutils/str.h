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

  void append(const char *str);
  void append(const Str &str);
  
  void add(char c);
  void set(char c, int i);
  void clear();
  
  int capacity() const;
  int len() const;
  
  void expand(int capacity);

  const char *c_str() const;
  
 private:
  char *buf;
  int cap;
};

inline Str::Str() : buf(0), cap(0) {}
inline const char *Str::c_str() const {return buf;}
inline int Str::capacity() const {return cap;}
inline void Str::set(char c, int i) {buf[i] = c;}

#endif
