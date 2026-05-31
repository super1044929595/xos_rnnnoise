#ifndef RL_SDK_BRIDGE_H
#define RL_SDK_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void RL_SDK_Bridge_Init(void);
void RL_SDK_Bridge_OnPwmTransferComplete(TIM_HandleTypeDef* htim);
void RL_SDK_Bridge_SetDiagColor(uint8_t g, uint8_t r, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif /* RL_SDK_BRIDGE_H */
