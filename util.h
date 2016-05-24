
#ifndef UTIL_H 
#define UTIL_H 

#define streq(X,Y)       (strcmp((X),(Y))==0)
#define startswith(X,P)  (strncmp((X), (P), strlen(P)) == 0)

#define likely(x)        __builtin_expect((x),1)
#define unlikely(x)      __builtin_expect((x),0)

int file_exists(const char *path);
long file_size(const char *path);
void message(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
double microtime();

#endif  /* UTIL_H  */
