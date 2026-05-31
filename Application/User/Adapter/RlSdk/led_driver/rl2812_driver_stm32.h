#ifndef __RL2812_DRIVER_STM32_H__
#define __RL2812_DRIVER_STM32_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_HAL_DRIVER)
#include "stm32f4xx_hal.h"
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * STM32 backend switch:
 *   Define RL2812_BACKEND_STM32 before including rl2812_driver.h
 *
 * Integration notes:
 * 1. Include your STM32 HAL headers in the build unit that compiles
 *    rl2812_driver_stm32.c, or provide them through rl2812_driver_stm32_port.h.
 * 2. Call RL2812_STM32_AttachTimerDma() after creating the strip, or
 *    RL2812_STM32H7_AttachTimerDmaAuto() if you want ARR/duty auto-calculation.
 * 3. In HAL_TIM_PWM_PulseFinishedCallback() or your DMA completion callback,
 *    call RL2812_STM32_TxCpltCallback().
 */

#ifndef RL2812_STM32_LED_NUM_MAX
#define RL2812_STM32_LED_NUM_MAX        64U
#endif

#ifndef RL2812_STM32_BITS_PER_LED
#define RL2812_STM32_BITS_PER_LED       24U
#endif

#ifndef RL2812_STM32_RESET_SLOTS
#define RL2812_STM32_RESET_SLOTS        64U
#endif

#ifndef RL2812_STM32_BRIGHTNESS_MAX
#define RL2812_STM32_BRIGHTNESS_MAX     100U
#endif

#if !defined(USE_HAL_DRIVER)
typedef struct TIM_HandleTypeDef TIM_HandleTypeDef;
typedef struct DMA_HandleTypeDef DMA_HandleTypeDef;
#endif

typedef struct _RL2812_STRIP
{
    uint16_t            led_num;
    uint8_t             brightness;
    uint8_t*            color_buf;        /* Raw color buffer, GRB per LED */
    uint16_t*           pwm_buf;          /* Timer compare buffer for DMA */
    uint32_t            pwm_buf_len;
    uint16_t            pwm_duty_0;
    uint16_t            pwm_duty_1;
    TIM_HandleTypeDef*  htim;
    DMA_HandleTypeDef*  hdma;
    uint32_t            tim_channel;
    uint32_t            timer_clk_hz;
    uint16_t            arr;
    volatile uint8_t    tx_done;
    volatile uint8_t    tx_busy;
    uint8_t             attached;
} RL2812_STRIP;

RL2812_STRIP* RL2812_A13_Init(uint16_t led_num);
RL2812_STRIP* RL2812_B18_Init(uint16_t led_num);
void RL2812_DeInit(RL2812_STRIP* strip);

void RL2812_SetPixel(RL2812_STRIP* strip, uint16_t led_index, uint8_t g, uint8_t r, uint8_t b);
void RL2812_SetPixelColor(RL2812_STRIP* strip, uint16_t led_index, uint32_t color);
void RL2812_SetAll(RL2812_STRIP* strip, uint8_t g, uint8_t r, uint8_t b);
void RL2812_Clear(RL2812_STRIP* strip);

void RL2812_SetBrightness(RL2812_STRIP* strip, uint8_t brightness);
uint8_t RL2812_GetBrightness(RL2812_STRIP* strip);
void RL2812_ApplyBrightness(RL2812_STRIP* strip);

void RL2812_SendData(RL2812_STRIP* strip);

/*
 * Platform attach points.
 * User must bind timer/dma/channel after strip init.
 */
void RL2812_STM32_AttachTimerDma(RL2812_STRIP* strip,
                                 TIM_HandleTypeDef* htim,
                                 DMA_HandleTypeDef* hdma,
                                 uint32_t tim_channel,
                                 uint16_t pwm_duty_0,
                                 uint16_t pwm_duty_1);

/*
 * H7 helper:
 * Compute ARR / duty values for 800kHz WS2812 waveform from timer source clock.
 * Typical T0H ~= 0.35us, T1H ~= 0.70us, period = 1.25us.
 */
void RL2812_STM32H7_AttachTimerDmaAuto(RL2812_STRIP* strip,
                                       TIM_HandleTypeDef* htim,
                                       DMA_HandleTypeDef* hdma,
                                       uint32_t tim_channel,
                                       uint32_t timer_clk_hz);

/*
 * To be called from HAL_TIM_PWM_PulseFinishedCallback / DMA complete callback.
 */
void RL2812_STM32_TxCpltCallback(RL2812_STRIP* strip);

#ifdef __cplusplus
}
#endif

#endif
