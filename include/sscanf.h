#ifndef SSCANF_H
#define SSCANF_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <c_types.h>
#include <stdint.h>


// Prototypes
size_t strcspn (const char *, const char *);
char * _getbase(char *, int *);
int _atob (uint32_t *, char *, int);
int atob(uint32_t *, char *, int);
int vsscanf (const char *, const char *, va_list);
int sscanf (const char *, const char *, ...);

#endif