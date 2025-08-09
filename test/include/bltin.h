#ifndef _BLTIN_H_
#define _BLTIN_H_

void * __builtin_malloc(unsigned int);
void * __builtin_calloc(unsigned int);
void __builtin_free(void*);
double __builtin_huge_val();
bool __builtin_isnan(double);
int __builtin_ilogb(double);
double __builtin_inf();
void __builtin_memset(void*, unsigned int, unsigned int);
unsigned int __builtin_strlen(char const*);
void * __builtin_alloca(unsigned int);

#endif
