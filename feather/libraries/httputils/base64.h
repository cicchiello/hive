#ifndef base64_h
#define base64_h

class StrBuf;

int base64_encode(StrBuf *output, const char *input, int inputLen);

// more memory efficient interface for those clients that don't have the credentials
// as one string.  "inputs" should be defined like:
//     static const char *ins[] = {"foo", ":", "bar", 0};
int base64_encode(StrBuf *output, const char *inputs[]);

#endif
