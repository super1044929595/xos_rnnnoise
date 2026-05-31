#include "rl_sdk_bridge.h"

#include "main.h"
#include "rl2812_driver_stm32.h"
#include "rl_minerva.h"

extern TIM_HandleTypeDef htim3;
extern DMA_HandleTypeDef hdma_tim3_ch3;

static RL2812_STRIP* g_status_strip;
static uint8_t g_minerva_ready;
static RL_UI_LED_Effect_Mode g_led_effect = RL_UI_LED_EFFECT_NONE;

static void RL_SDK_Bridge_ShowFrame(uint8_t active_index, uint8_t g, uint8_t r, uint8_t b)
{
    RL2812_Clear(g_status_strip);
    RL2812_SetPixel(g_status_strip, active_index, g, r, b);
    RL2812_SendData(g_status_strip);
}

static void RL_SDK_Bridge_ShowBootPattern(void)
{
    if (g_status_strip == NULL)
    {
        return;
    }

    /* Red -> Green -> Blue chasing pixels for quick visual verification. */
    RL_SDK_Bridge_ShowFrame(0U, 0x00U, 0x30U, 0x00U);
    HAL_Delay(120U);
    RL_SDK_Bridge_ShowFrame(1U, 0x30U, 0x00U, 0x00U);
    HAL_Delay(120U);
    RL_SDK_Bridge_ShowFrame(2U, 0x00U, 0x00U, 0x30U);
    HAL_Delay(120U);
    RL_SDK_Bridge_ShowFrame(3U, 0x00U, 0x30U, 0x00U);
    HAL_Delay(120U);
    RL_SDK_Bridge_ShowFrame(4U, 0x30U, 0x00U, 0x00U);
    HAL_Delay(120U);
    RL_SDK_Bridge_ShowFrame(5U, 0x00U, 0x00U, 0x30U);
    HAL_Delay(120U);
    RL2812_Clear(g_status_strip);
}

void RL_SDK_Bridge_Init(void)
{
    if (g_status_strip != NULL)
    {
        return;
    }

    g_status_strip = RL2812_A13_Init(8U);
    if (g_status_strip == NULL)
    {
        Error_Handler();
    }

    /* TIM3 on APB1: APB1=50MHz, timer clk = APB1*2 = 100MHz (ST timer doubler) */
    RL2812_STM32H7_AttachTimerDmaAuto(g_status_strip,
                                      &htim3,
                                      &hdma_tim3_ch3,
                                      TIM_CHANNEL_3,
                                      100000000UL);

    if (RL_Minerva_Init())
    {
        g_minerva_ready = RL_Minerva_RunSelfTest() ? 1U : 0U;
    }

    RL2812_RGB_Init();

    RL_SDK_Bridge_ShowBootPattern();

    if (g_minerva_ready != 0U)
    {
        RL2812_SetPixel(g_status_strip, 3U, 0x00U, 0x20U, 0x00U);
        RL2812_SendData(g_status_strip);
    }
}

void RL_SDK_Bridge_OnPwmTransferComplete(TIM_HandleTypeDef* htim)
{
    if ((htim == NULL) || (htim->Instance != TIM3) || (g_status_strip == NULL))
    {
        return;
    }

    RL2812_STM32_TxCpltCallback(g_status_strip);
}

void RL_SDK_Bridge_SetDiagColor(uint8_t g, uint8_t r, uint8_t b)
{
    if (g_status_strip == NULL)
    {
        return;
    }

    RL2812_SetAll(g_status_strip, g, r, b);
    RL2812_SendData(g_status_strip);
}

void RL_SDK_Bridge_SetPixel(uint8_t index, uint8_t g, uint8_t r, uint8_t b)
{
    if (g_status_strip == NULL || index >= 8U)
    {
        return;
    }

    RL2812_SetPixel(g_status_strip, index, g, r, b);
    RL2812_SendData(g_status_strip);
}

void RL_SDK_Bridge_Clear(void)
{
    if (g_status_strip == NULL)
    {
        return;
    }

    RL2812_Clear(g_status_strip);
}

void RL_SDK_Bridge_SetLedEffect(RL_UI_LED_Effect_Mode effect)
{
    g_led_effect = effect;
}

void RL_SDK_Bridge_LedTimerUpdate(void)
{
    if (g_status_strip == NULL)
    {
        return;
    }

    RL2812_RGB_TimerUpdate(g_status_strip, g_led_effect);
}
