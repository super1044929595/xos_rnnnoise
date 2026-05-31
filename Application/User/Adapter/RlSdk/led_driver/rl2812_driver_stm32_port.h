#ifndef __RL2812_DRIVER_STM32_PORT_H__
#define __RL2812_DRIVER_STM32_PORT_H__

/*
 * STM32 HAL port layer.
 *
 * Usage:
 * 1. Make sure your project includes the proper STM32 HAL headers before
 *    compiling rl2812_driver_stm32.c, for example:
 *      #include "stm32h7xx_hal.h"
 *      #include "tim.h"
 *      #include "dma.h"
 *
 * 2. If your project uses standard HAL names, no extra macros are needed.
 *
 * 3. For STM32H7 with D-Cache enabled, define:
 *      #define RL2812_STM32_DCACHE_CLEAN(addr, size) \
 *          SCB_CleanDCache_by_Addr((uint32_t*)(addr), (int32_t)(size))
 *
 * 4. If your HAL wrapper names differ, override the macros below before
 *    compiling this driver.
 */

#if defined(USE_HAL_DRIVER)
#include "stm32f4xx_hal.h"
#endif

#ifndef RL2812_STM32_HAL_OK
#define RL2812_STM32_HAL_OK                      HAL_OK
#endif

#ifndef RL2812_STM32_HAL_TIM_PWM_START_DMA
#define RL2812_STM32_HAL_TIM_PWM_START_DMA      HAL_TIM_PWM_Start_DMA
#endif

#ifndef RL2812_STM32_HAL_TIM_PWM_STOP_DMA
#define RL2812_STM32_HAL_TIM_PWM_STOP_DMA       HAL_TIM_PWM_Stop_DMA
#endif

#ifndef RL2812_STM32_GET_AUTORELOAD
#define RL2812_STM32_GET_AUTORELOAD             __HAL_TIM_GET_AUTORELOAD
#endif

#ifndef RL2812_STM32_SET_AUTORELOAD
#define RL2812_STM32_SET_AUTORELOAD             __HAL_TIM_SET_AUTORELOAD
#endif

#ifndef RL2812_STM32_SET_COMPARE
#define RL2812_STM32_SET_COMPARE                __HAL_TIM_SET_COMPARE
#endif

#ifndef RL2812_STM32_DCACHE_CLEAN
#define RL2812_STM32_DCACHE_CLEAN(addr, size)   ((void)0)
#endif

#endif
