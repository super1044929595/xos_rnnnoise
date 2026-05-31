#include "rl2812_driver_stm32.h"

#include <string.h>
#include <stdlib.h>
#include "main.h"

#ifndef RL2812_STM32_USE_HAL
#define RL2812_STM32_USE_HAL 1
#endif

#if RL2812_STM32_USE_HAL
#include "rl2812_driver_stm32_port.h"
#endif

#ifndef RL2812_STM32_TX_TIMEOUT
#define RL2812_STM32_TX_TIMEOUT      10000000UL
#endif

static RL2812_STRIP rl2812_strips[2];
static uint8_t rl2812_init_flag[2] = {0U, 0U};

static uint32_t RL2812_STM32_CalcPwmBufLen(uint16_t led_num)
{
    return ((uint32_t)led_num * RL2812_STM32_BITS_PER_LED) + RL2812_STM32_RESET_SLOTS;
}

static void RL2812_STM32_EncodePixel(RL2812_STRIP* strip, uint16_t led_index, uint8_t g, uint8_t r, uint8_t b)
{
    uint32_t bit_base;
    uint8_t colors[3];

    if(strip == NULL || strip->pwm_buf == NULL || led_index >= strip->led_num){
        return;
    }

    if(strip->brightness < RL2812_STM32_BRIGHTNESS_MAX){
        g = (uint8_t)(((uint16_t)g * strip->brightness) / RL2812_STM32_BRIGHTNESS_MAX);
        r = (uint8_t)(((uint16_t)r * strip->brightness) / RL2812_STM32_BRIGHTNESS_MAX);
        b = (uint8_t)(((uint16_t)b * strip->brightness) / RL2812_STM32_BRIGHTNESS_MAX);
    }

    bit_base = (uint32_t)led_index * RL2812_STM32_BITS_PER_LED;
    colors[0] = g;
    colors[1] = r;
    colors[2] = b;

    for(uint32_t color_idx = 0U; color_idx < 3U; color_idx++){
        uint8_t value = colors[color_idx];
        for(int8_t bit = 7; bit >= 0; bit--){
            strip->pwm_buf[bit_base++] = (value & (1U << bit)) ? strip->pwm_duty_1 : strip->pwm_duty_0;
        }
    }
}

static void RL2812_STM32_RebuildFromColorBuf(RL2812_STRIP* strip)
{
    if(strip == NULL || strip->pwm_buf == NULL || strip->color_buf == NULL){
        return;
    }

    memset(strip->pwm_buf, 0, strip->pwm_buf_len * sizeof(uint16_t));
    for(uint16_t i = 0U; i < strip->led_num; i++){
        uint32_t color_idx = (uint32_t)i * 3U;
        RL2812_STM32_EncodePixel(strip, i,
                                 strip->color_buf[color_idx + 0U],
                                 strip->color_buf[color_idx + 1U],
                                 strip->color_buf[color_idx + 2U]);
    }
}

static uint16_t RL2812_STM32H7_CalcArr(uint32_t timer_clk_hz)
{
    uint32_t arr_plus_1;

    if(timer_clk_hz == 0U){
        return 0U;
    }

    arr_plus_1 = (timer_clk_hz + 400000UL) / 800000UL;
    if(arr_plus_1 == 0U){
        arr_plus_1 = 1U;
    }
    if(arr_plus_1 > 0x10000UL){
        arr_plus_1 = 0x10000UL;
    }
    return (uint16_t)(arr_plus_1 - 1U);
}

static uint16_t RL2812_STM32H7_CalcDutyFromNs(uint16_t arr, uint32_t high_ns)
{
    uint32_t period_counts = (uint32_t)arr + 1U;
    uint32_t duty = (period_counts * high_ns + 625U) / 1250U;

    if(duty > arr){
        duty = arr;
    }
    return (uint16_t)duty;
}

static RL2812_STRIP* RL2812_STM32_InitCommon(RL2812_STRIP* strip, uint8_t* init_flag, uint16_t led_num)
{
    if(*init_flag != 0U){
        return strip;
    }

    if(led_num == 0U || led_num > RL2812_STM32_LED_NUM_MAX){
        return NULL;
    }

    memset(strip, 0, sizeof(RL2812_STRIP));
    strip->led_num = led_num;
    strip->brightness = RL2812_STM32_BRIGHTNESS_MAX;
    strip->pwm_buf_len = RL2812_STM32_CalcPwmBufLen(led_num);

    strip->color_buf = (uint8_t*)malloc((size_t)led_num * 3U);
    if(strip->color_buf == NULL){
        return NULL;
    }
    memset(strip->color_buf, 0, (size_t)led_num * 3U);

    strip->pwm_buf = (uint16_t*)malloc((size_t)strip->pwm_buf_len * sizeof(uint16_t));
    if(strip->pwm_buf == NULL){
        free(strip->color_buf);
        strip->color_buf = NULL;
        return NULL;
    }
    memset(strip->pwm_buf, 0, (size_t)strip->pwm_buf_len * sizeof(uint16_t));

    *init_flag = 1U;
    return strip;
}

RL2812_STRIP* RL2812_A13_Init(uint16_t led_num)
{
    return RL2812_STM32_InitCommon(&rl2812_strips[0], &rl2812_init_flag[0], led_num);
}

RL2812_STRIP* RL2812_B18_Init(uint16_t led_num)
{
    return RL2812_STM32_InitCommon(&rl2812_strips[1], &rl2812_init_flag[1], led_num);
}

void RL2812_DeInit(RL2812_STRIP* strip)
{
    if(strip == NULL){
        return;
    }

    if(strip->color_buf != NULL){
        free(strip->color_buf);
        strip->color_buf = NULL;
    }

    if(strip->pwm_buf != NULL){
        free(strip->pwm_buf);
        strip->pwm_buf = NULL;
    }

    strip->pwm_buf_len = 0U;
    strip->attached = 0U;
    strip->tx_done = 0U;

    if(strip == &rl2812_strips[0]){
        rl2812_init_flag[0] = 0U;
    }else if(strip == &rl2812_strips[1]){
        rl2812_init_flag[1] = 0U;
    }
}

void RL2812_SetPixel(RL2812_STRIP* strip, uint16_t led_index, uint8_t g, uint8_t r, uint8_t b)
{
    uint32_t color_idx;

    if(strip == NULL || led_index >= strip->led_num || strip->color_buf == NULL){
        return;
    }

    color_idx = (uint32_t)led_index * 3U;
    strip->color_buf[color_idx + 0U] = g;
    strip->color_buf[color_idx + 1U] = r;
    strip->color_buf[color_idx + 2U] = b;

    RL2812_STM32_EncodePixel(strip, led_index, g, r, b);
}

void RL2812_SetPixelColor(RL2812_STRIP* strip, uint16_t led_index, uint32_t color)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    if(strip == NULL){
        return;
    }

    r = (uint8_t)((color >> 16) & 0xFFU);
    g = (uint8_t)((color >> 8) & 0xFFU);
    b = (uint8_t)(color & 0xFFU);
    RL2812_SetPixel(strip, led_index, g, r, b);
}

void RL2812_SetAll(RL2812_STRIP* strip, uint8_t g, uint8_t r, uint8_t b)
{
    if(strip == NULL){
        return;
    }

    for(uint16_t i = 0U; i < strip->led_num; i++){
        RL2812_SetPixel(strip, i, g, r, b);
    }
}

void RL2812_Clear(RL2812_STRIP* strip)
{
    if(strip == NULL){
        return;
    }

    RL2812_SetAll(strip, 0U, 0U, 0U);
    RL2812_SendData(strip);
}

void RL2812_SetBrightness(RL2812_STRIP* strip, uint8_t brightness)
{
    if(strip == NULL){
        return;
    }

    if(brightness > RL2812_STM32_BRIGHTNESS_MAX){
        brightness = RL2812_STM32_BRIGHTNESS_MAX;
    }
    strip->brightness = brightness;
}

uint8_t RL2812_GetBrightness(RL2812_STRIP* strip)
{
    if(strip == NULL){
        return 0U;
    }
    return strip->brightness;
}

void RL2812_ApplyBrightness(RL2812_STRIP* strip)
{
    if(strip == NULL){
        return;
    }

    RL2812_STM32_RebuildFromColorBuf(strip);
    RL2812_SendData(strip);
}

void RL2812_STM32_AttachTimerDma(RL2812_STRIP* strip,
                                 TIM_HandleTypeDef* htim,
                                 DMA_HandleTypeDef* hdma,
                                 uint32_t tim_channel,
                                 uint16_t pwm_duty_0,
                                 uint16_t pwm_duty_1)
{
    if(strip == NULL){
        return;
    }

    strip->htim = htim;
    strip->hdma = hdma;
    strip->tim_channel = tim_channel;
    strip->pwm_duty_0 = pwm_duty_0;
    strip->pwm_duty_1 = pwm_duty_1;
    strip->arr = (htim != NULL) ? (uint16_t)RL2812_STM32_GET_AUTORELOAD(htim) : 0U;
    strip->attached = 1U;
    strip->tx_done = 1U;
    strip->tx_busy = 0U;

    RL2812_STM32_RebuildFromColorBuf(strip);
}

void RL2812_STM32H7_AttachTimerDmaAuto(RL2812_STRIP* strip,
                                       TIM_HandleTypeDef* htim,
                                       DMA_HandleTypeDef* hdma,
                                       uint32_t tim_channel,
                                       uint32_t timer_clk_hz)
{
    uint16_t arr;
    uint16_t duty_0;
    uint16_t duty_1;

    if(strip == NULL || htim == NULL){
        return;
    }

    arr = RL2812_STM32H7_CalcArr(timer_clk_hz);
    duty_0 = RL2812_STM32H7_CalcDutyFromNs(arr, 350U);
    duty_1 = RL2812_STM32H7_CalcDutyFromNs(arr, 700U);

    RL2812_STM32_SET_AUTORELOAD(htim, arr);
    RL2812_STM32_SET_COMPARE(htim, tim_channel, 0U);

    strip->timer_clk_hz = timer_clk_hz;
    strip->arr = arr;

    RL2812_STM32_AttachTimerDma(strip, htim, hdma, tim_channel, duty_0, duty_1);
}

void RL2812_STM32_TxCpltCallback(RL2812_STRIP* strip)
{
    if(strip == NULL){
        return;
    }
    strip->tx_done = 1U;
    strip->tx_busy = 0U;
}

void RL2812_SendData(RL2812_STRIP* strip)
{
    uint32_t timeout;

    if(strip == NULL || strip->attached == 0U || strip->htim == NULL || strip->pwm_buf == NULL){
        return;
    }

    if(strip->tx_busy != 0U){
        return;
    }

#if RL2812_STM32_USE_HAL
    RL2812_STM32_DCACHE_CLEAN(strip->pwm_buf, strip->pwm_buf_len * sizeof(uint16_t));
    strip->tx_done = 0U;
    strip->tx_busy = 1U;
    if(RL2812_STM32_HAL_TIM_PWM_START_DMA(strip->htim, strip->tim_channel, (uint32_t*)strip->pwm_buf, strip->pwm_buf_len) != RL2812_STM32_HAL_OK){
        strip->tx_done = 1U;
        strip->tx_busy = 0U;
        return;
    }

    timeout = RL2812_STM32_TX_TIMEOUT;
    while(strip->tx_done == 0U && timeout > 0U){
        timeout--;
    }
    if(timeout == 0U){
        strip->tx_busy = 0U;
    }
    RL2812_STM32_HAL_TIM_PWM_STOP_DMA(strip->htim, strip->tim_channel);
    strip->tx_done = 1U;
    strip->tx_busy = 0U;
#endif
}
