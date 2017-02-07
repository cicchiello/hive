#ifndef http_dataprovider_h
#define http_dataprovider_h

class HttpDataProvider {
private:
public:
  HttpDataProvider() {}
  virtual ~HttpDataProvider();

  virtual int takeIfAvailable(unsigned char **buf) = 0;

  virtual bool isDone() const = 0;
  
  virtual int getSize() const = 0;

  virtual void close() = 0;

  virtual void start() = 0;

  virtual void giveBack(unsigned char *) {}

  virtual void reset() = 0;
};


#endif
