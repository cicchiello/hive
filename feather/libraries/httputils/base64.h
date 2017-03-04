#ifndef base64_h
#define base64_h

class StrBuf;

int base64_encode(StrBuf *output, const char *input, int inputLen);

#endif
