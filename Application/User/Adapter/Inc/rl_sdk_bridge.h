#ifndef RL_SDK_BRIDGE_H
#define RL_SDK_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "rl2812_driver_stm32.h"

void RL_SDK_Bridge_Init(void);
void RL_SDK_Bridge_OnPwmTransferComplete(TIM_HandleTypeDef* htim);
void RL_SDK_Bridge_SetDiagColor(uint8_t g, uint8_t r, uint8_t b);
void RL_SDK_Bridge_SetPixel(uint8_t index, uint8_t g, uint8_t r, uint8_t b);
void RL_SDK_Bridge_Clear(void);

/* LED effect control */
void RL_SDK_Bridge_SetLedEffect(RL_UI_LED_Effect_Mode effect);
void RL_SDK_Bridge_LedTimerUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* RL_SDK_BRIDGE_H */
