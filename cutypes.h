#ifndef CUTYPES_H
#define CUTYPES_H

#include <stdlib.h>

#define CLEANUP_FUNC(type, func)                \
    static inline void func##type(type **value) \
    {                                           \
        if (*value != NULL)                     \
        {                                       \
            free(*value);                       \
            *value = NULL;                      \
        }                                       \
    }                                           \
    struct __allow_semicolon__

CLEANUP_FUNC(char, free);
#define cu_char __attribute__((cleanup(freechar))) char

#endif