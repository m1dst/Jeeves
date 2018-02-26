#ifndef SSCANF_H
#define SSCANF_H
 
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <cstdint>
 
 
// Prototypes
size_t strcspn (const char *, const char *);
char * _getbase(char *, int *);
int _atob (uint32_t *, char *, int);
int atob(uint32_t *, char *, int);
int vsscanf (const char *, const char *, va_list);
int sscanf (const char *, const char *, ...);
 
#endif
