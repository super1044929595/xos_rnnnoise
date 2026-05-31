#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifndef APP_DBG
#define APP_DBG(...) ((void)printf(__VA_ARGS__))
#endif

#endif /* DEBUG_H */
