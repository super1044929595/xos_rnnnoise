#ifndef RL_MINERVA_KWS_H
#define RL_MINERVA_KWS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "audio_core_api.h"

typedef enum
{
    RL_MINERVA_KWS_STATE_IDLE = 0,
    RL_MINERVA_KWS_STATE_READY,
    RL_MINERVA_KWS_STATE_TRIGGERED,
} RL_MinervaKwsState;

typedef struct
{
    RL_MinervaKwsState state;
    uint8_t            keyword_index;
    uint8_t            confidence;
    uint32_t           frame_count;
    uint32_t           dropped_samples;
} RL_MinervaKwsResult;

bool RL_MinervaKws_Init(void);
void RL_MinervaKws_Reset(void);
void RL_MinervaKws_AudioFramePush(const PCM_DATA_TYPE *pcm,
                                  uint16_t             samples,
                                  uint8_t              channels,
                                  uint8_t              width);
void RL_MinervaKws_Process(void);
RL_MinervaKwsResult RL_MinervaKws_GetResult(void);

#ifdef __cplusplus
}
#endif

#endif /* RL_MINERVA_KWS_H */
