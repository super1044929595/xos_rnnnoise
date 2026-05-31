#ifndef DEBUG_H
#define DEBUG_H

// 独立运行时关闭 Semihosting printf，避免 BKPT 0xAB 卡死
// 调试时取消下面注释可恢复 printf 输出
// #define ENABLE_DBG_PRINTF

#ifdef ENABLE_DBG_PRINTF
#include <stdio.h>
#define APP_DBG(...) ((void)printf(__VA_ARGS__))
#else
#define APP_DBG(...) ((void)0)
#endif

#endif /* DEBUG_H */
