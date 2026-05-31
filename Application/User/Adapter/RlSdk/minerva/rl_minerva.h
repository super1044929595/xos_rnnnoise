#ifndef RL_MINERVA_H
#define RL_MINERVA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "minerva.h"

bool RL_Minerva_HasModel(void);
bool RL_Minerva_Init(void);
void RL_Minerva_Deinit(void);
bool RL_Minerva_IsReady(void);
void RL_Minerva_Seed(uint32_t seed);
bool RL_Minerva_RunSelfTest(void);

mnv_status_t RL_Minerva_Run(const mnv_act_t *input,
                            uint16_t         input_len,
                            mnv_act_t       *output,
                            uint16_t         output_len);

mnv_status_t RL_Minerva_VerifyModel(void);
void RL_Minerva_GetOutputMac(uint8_t *mac, uint16_t mac_len);

const mnv_model_t *RL_Minerva_GetModel(void);
const mnv_ctx_t   *RL_Minerva_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif /* RL_MINERVA_H */
