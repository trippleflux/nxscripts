/* ---- BASE64 Routines ---- */
#ifdef BASE64
int base64_encode(const unsigned char *in,  unsigned long len,
                        unsigned char *out, unsigned long *outlen);

int base64_decode(const unsigned char *in,  unsigned long len,
                        unsigned char *out, unsigned long *outlen);
#endif

void zeromem(void *dst, size_t len);
void burn_stack(unsigned long len);
const char *error_to_string(int err);
