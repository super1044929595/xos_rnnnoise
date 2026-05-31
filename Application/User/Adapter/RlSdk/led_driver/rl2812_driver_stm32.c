#include "rl2812_driver_stm32.h"

#include <string.h>
#include <stdlib.h>

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

/*=============================================================================
 * LED Effect: Global State
 *============================================================================*/
RL_RGB_BreathInfo rl_rgb_breathinfo;
RL_RGB_MarqueInfo rl_rgb_marqueinfo;
RL_RGB_MoveInfo   rl_rgb_moveinfo;
RL_RGB_FadeInfo   rl_rgb_fadeinfo;

static RL_UI_LED_Effect_Mode s_last_led_effect = RL_UI_LED_EFFECT_NONE;

/*=============================================================================
 * LED Effect: Utility Functions
 *============================================================================*/

void RL2812_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b)
{
    float hh, p, q, t, ff;
    long i;

    if(r == NULL || g == NULL || b == NULL){
        return;
    }

    if(s == 0){
        *r = v;
        *g = v;
        *b = v;
        return;
    }

    hh = (float)h / 360.0f * 6.0f;
    i = (long)hh;
    ff = hh - (float)i;
    p = (float)v * (1.0f - (float)s / 100.0f);
    q = (float)v * (1.0f - (float)s / 100.0f * ff);
    t = (float)v * (1.0f - (float)s / 100.0f * (1.0f - ff));

    switch(i){
        case 0: *r = v;        *g = (uint8_t)t; *b = (uint8_t)p; break;
        case 1: *r = (uint8_t)q; *g = v;        *b = (uint8_t)p; break;
        case 2: *r = (uint8_t)p; *g = v;        *b = (uint8_t)t; break;
        case 3: *r = (uint8_t)p; *g = (uint8_t)q; *b = v;        break;
        case 4: *r = (uint8_t)t; *g = (uint8_t)p; *b = v;        break;
        case 5: *r = v;        *g = (uint8_t)p; *b = (uint8_t)q; break;
        default: break;
    }
}

/*=============================================================================
 * LED Effect: Init
 *============================================================================*/
void RL2812_RGB_Init(void)
{
    memset(&rl_rgb_breathinfo, 0, sizeof(RL_RGB_BreathInfo));
    memset(&rl_rgb_marqueinfo, 0, sizeof(RL_RGB_MarqueInfo));
    memset(&rl_rgb_moveinfo,    0, sizeof(RL_RGB_MoveInfo));
    memset(&rl_rgb_fadeinfo,    0, sizeof(RL_RGB_FadeInfo));

    rl_rgb_breathinfo.g = 0;
    rl_rgb_breathinfo.r = 80;
    rl_rgb_breathinfo.b = 0;
    rl_rgb_breathinfo.brightness = 0;
    rl_rgb_breathinfo.enable = 0;
    rl_rgb_breathinfo.cycle_count = 0;

    rl_rgb_marqueinfo.position = 0;
    rl_rgb_marqueinfo.cycle_count = 0;
    rl_rgb_moveinfo.state = 0;
    rl_rgb_moveinfo.basecnt = 0;
    rl_rgb_fadeinfo.hue = 0;
    rl_rgb_fadeinfo.position = 0;
    rl_rgb_fadeinfo.basecnt = 0;

    s_last_led_effect = RL_UI_LED_EFFECT_NONE;
}

/*=============================================================================
 * LED Effect: Timer Update (call periodically, e.g. every 10ms)
 *============================================================================*/
void RL2812_RGB_TimerUpdate(RL2812_STRIP* strip, RL_UI_LED_Effect_Mode led_effect)
{
    if(strip == NULL){
        return;
    }

    /* Reset effect state when effect mode changes */
    if(s_last_led_effect != led_effect){
        rl_rgb_breathinfo.brightness = 0;
        rl_rgb_breathinfo.enable = 0;
        rl_rgb_breathinfo.cycle_count = 0;
        rl_rgb_marqueinfo.position = 0;
        rl_rgb_marqueinfo.cycle_count = 0;
        rl_rgb_moveinfo.state = 0;
        rl_rgb_moveinfo.basecnt = 0;
        rl_rgb_fadeinfo.hue = 0;
        rl_rgb_fadeinfo.position = 0;
        rl_rgb_fadeinfo.basecnt = 0;
        s_last_led_effect = led_effect;
    }

    switch(led_effect)
    {
        case RL_UI_LED_EFFECT_NONE:
            return;

        case RL_UI_LED_EFFECT_BREATH:
            if((rl_rgb_breathinfo.cycle_count++ % 2) != 0){
                return;
            }
            if(rl_rgb_breathinfo.enable == 0){
                if(rl_rgb_breathinfo.brightness < MAX_BRIGHTNESS){
                    rl_rgb_breathinfo.brightness += 2;
                }else{
                    rl_rgb_breathinfo.enable = 1;
                }
            }else{
                if(rl_rgb_breathinfo.brightness > 2){
                    rl_rgb_breathinfo.brightness -= 2;
                }else{
                    rl_rgb_breathinfo.brightness = 0;
                    rl_rgb_breathinfo.enable = 0;
                }
            }
            rl_rgb_breathinfo.step = (float)rl_rgb_breathinfo.brightness / (float)MAX_BRIGHTNESS;
            RL2812_SetAll(strip,
                (uint8_t)((float)rl_rgb_breathinfo.g * rl_rgb_breathinfo.step),
                (uint8_t)((float)rl_rgb_breathinfo.r * rl_rgb_breathinfo.step),
                (uint8_t)((float)rl_rgb_breathinfo.b * rl_rgb_breathinfo.step));
            RL2812_SendData(strip);
            break;

        case RL_UI_LED_EFFECT_MARQUEE:
            if((rl_rgb_marqueinfo.cycle_count++ % 3) != 0){
                return;
            }
            for(uint16_t i = 0; i < strip->led_num; i++){
                uint16_t led_hue = (uint16_t)((rl_rgb_marqueinfo.position * 18U + i * 360U / strip->led_num) % 360U);
                uint8_t r, g, b;
                RL2812_HSVtoRGB(led_hue, 100, 25, &r, &g, &b);
                RL2812_SetPixel(strip, i, g, r, b);
            }
            RL2812_SendData(strip);
            rl_rgb_marqueinfo.position++;
            if(rl_rgb_marqueinfo.position >= 20U){
                rl_rgb_marqueinfo.position = 0;
            }
            break;

        case RL_UI_LED_EFFECT_I2S_FLASH:
            if((rl_rgb_moveinfo.basecnt++ % 2U) != 0U){
                return;
            }
            {
                /* Slow down: only advance state every N visual frames */
                static uint8_t i2s_slow_cnt = 0U;
                i2s_slow_cnt++;
                if(i2s_slow_cnt < 4U){
                    return;
                }
                i2s_slow_cnt = 0U;
                uint16_t half_leds = strip->led_num / 2U;
                uint16_t sweep_steps = (strip->led_num + 1U) / 2U;

                if(rl_rgb_moveinfo.state < sweep_steps){
                    RL2812_SetAll(strip, 0, 0, 0);
                    for(uint16_t d = 0; d <= rl_rgb_moveinfo.state; d++){
                        uint8_t v = 0U;
                        uint8_t r, g, b;
                        uint16_t lit_count = (uint16_t)rl_rgb_moveinfo.state + 1U;
                        uint16_t left_idx;
                        uint16_t right_idx;

                        if(d + 1U == lit_count){
                            v = 100U;
                        }else if(d + 2U == lit_count){
                            v = 72U;
                        }else if(d + 3U == lit_count){
                            v = 45U;
                        }else{
                            v = 26U;
                        }

                        /* Audi-style sequential sweep, fixed cool-white RGB */
                        r = v;
                        g = v;
                        b = (uint8_t)((uint16_t)v * 92U / 100U);

                        if((strip->led_num & 0x1U) != 0U){
                            if(d == 0U){
                                RL2812_SetPixel(strip, half_leds, g, r, b);
                                continue;
                            }
                            left_idx = half_leds - d;
                            right_idx = half_leds + d;
                        }else{
                            left_idx = (half_leds - 1U) - d;
                            right_idx = half_leds + d;
                        }

                        RL2812_SetPixel(strip, left_idx, g, r, b);
                        if(right_idx < strip->led_num){
                            RL2812_SetPixel(strip, right_idx, g, r, b);
                        }
                    }
                    RL2812_SendData(strip);
                }else if(rl_rgb_moveinfo.state == sweep_steps){
                    RL2812_SetAll(strip, 100U, 100U, 100U);
                    RL2812_SendData(strip);
                }else{
                    RL2812_SetAll(strip, 0U, 0U, 0U);
                    RL2812_SendData(strip);
                }
                rl_rgb_moveinfo.state++;
                if(rl_rgb_moveinfo.state > (sweep_steps + 1U)){
                    rl_rgb_moveinfo.state = 0U;
                }
            }
            break;

        case RL_UI_LED_EFFECT_ALLON:
            if((rl_rgb_moveinfo.basecnt++ % 2U) != 0U){
                return;
            }
            RL2812_SetAll(strip, 0, 0, 0);
            for(uint16_t i = 0; i < strip->led_num; i++){
                uint16_t pos = (uint16_t)((i + rl_rgb_moveinfo.state) % strip->led_num);
                uint8_t r, g, b;
                uint8_t v;

                if(i == 0U){
                    v = 100U;
                }else if(i == 1U){
                    v = 55U;
                }else if(i == 2U){
                    v = 25U;
                }else{
                    v = 0U;
                }

                if(v == 0U){
                    continue;
                }

                RL2812_HSVtoRGB((uint16_t)((pos * 360U) / strip->led_num), 100, v, &r, &g, &b);
                RL2812_SetPixel(strip, pos, g, r, b);
            }
            RL2812_SendData(strip);
            rl_rgb_moveinfo.state++;
            if(rl_rgb_moveinfo.state >= strip->led_num){
                rl_rgb_moveinfo.state = 0U;
            }
            break;

        case RL_UI_LED_EFFECT_RAINBOW:
            for(uint16_t i = 0; i < strip->led_num; i++){
                uint16_t led_hue = (rl_rgb_fadeinfo.hue + (i * 360U / strip->led_num)) % 360U;
                uint8_t r, g, b;
                RL2812_HSVtoRGB(led_hue, 100, 60, &r, &g, &b);
                RL2812_SetPixel(strip, i, g, r, b);
            }
            RL2812_SendData(strip);
            rl_rgb_fadeinfo.hue = (rl_rgb_fadeinfo.hue + 6) % 360;
            break;

        case RL_UI_LED_EFFECT_AUDIO:
            /* Audio spectrum effect requires FFT and I2S audio input.
             * Placeholder: solid blue all LEDs.
             * To enable full audio spectrum, integrate your FFT and I2S pipeline. */
            RL2812_SetAll(strip, 0U, 0U, 20U);
            RL2812_SendData(strip);
            break;

        default:
            break;
    }
}
