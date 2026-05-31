#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "app_config.h"
#include "debug.h"
#include "fft_api.h"
#include "rtos_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rl_minerva.h"
#include "rl_minerva_kws.h"
#include "mnv_ct.h"

#define RL_MINERVA_KWS_SAMPLE_RATE        48000U
#define RL_MINERVA_KWS_FRAME_SAMPLES      480U
#define RL_MINERVA_KWS_RING_SAMPLES       2048U
#define RL_MINERVA_KWS_INPUT_FEATURES     36U
#define RL_MINERVA_KWS_FFT_SIZE           512U
#define RL_MINERVA_KWS_DECIM_STRIDE       3U
#define RL_MINERVA_KWS_MFCC_COUNT         12U
#define RL_MINERVA_KWS_MEL_BINS           20U
#define RL_MINERVA_KWS_MEL_SUMMARY        16U
#define RL_MINERVA_KWS_TRIGGER_THRESHOLD  60U
#define RL_MINERVA_KWS_SILENCE_BASELINE   14U
#define RL_MINERVA_KWS_SILENCE_MARGIN     8U
#define RL_MINERVA_KWS_SMOOTH_SHIFT       2U
#define RL_MINERVA_KWS_HIT_COUNT          3U
#define RL_MINERVA_KWS_COOLDOWN_FRAMES    12U
#define RL_MINERVA_KWS_TASK_STACK_SIZE    512U
#define RL_MINERVA_KWS_TASK_PRIO          2U

static const uint16_t s_rl_minerva_mel_edges[RL_MINERVA_KWS_MEL_BINS + 2U] = {
    9U, 13U, 16U, 21U, 25U, 31U, 37U, 43U, 50U, 58U, 67U,
    77U, 87U, 99U, 113U, 127U, 144U, 162U, 182U, 204U, 229U, 256U
};

static const int16_t s_rl_minerva_dct_q8[RL_MINERVA_KWS_MFCC_COUNT][RL_MINERVA_KWS_MEL_BINS] = {
    { 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 },
    { 255, 249, 237, 218, 195, 166, 134, 98, 60, 20, -20, -60, -98, -134, -166, -195, -218, -237, -249, -255 },
    { 253, 228, 181, 116, 40, -40, -116, -181, -228, -253, -253, -228, -181, -116, -40, 40, 116, 181, 228, 253 },
    { 249, 195, 98, -20, -134, -218, -255, -237, -166, -60, 60, 166, 237, 255, 218, 134, 20, -98, -195, -249 },
    { 243, 150, 0, -150, -243, -243, -150, 0, 150, 243, 243, 150, 0, -150, -243, -243, -150, 0, 150, 243 },
    { 237, 98, -98, -237, -237, -98, 98, 237, 237, 98, -98, -237, -237, -98, 98, 237, 237, 98, -98, -237 },
    { 228, 40, -181, -253, -116, 116, 253, 181, -40, -228, -228, -40, 181, 253, 116, -116, -253, -181, 40, 228 },
    { 218, -20, -237, -195, 60, 249, 166, -98, -255, -134, 134, 255, 98, -166, -249, -60, 195, 237, 20, -218 },
    { 207, -79, -256, -79, 207, 207, -79, -256, -79, 207, 207, -79, -256, -79, 207, 207, -79, -256, -79, 207 },
    { 195, -134, -237, 60, 255, 20, -249, -98, 218, 166, -166, -218, 98, 249, -20, -255, -60, 237, 134, -195 },
    { 181, -181, -181, 181, 181, -181, -181, 181, 181, -181, -181, 181, 181, -181, -181, 181, 181, -181, -181, 181 },
    { 166, -218, -98, 249, 20, -255, 60, 237, -134, -195, 195, 134, -237, -60, 255, -20, -249, 98, 218, -166 }
};

typedef struct
{
    bool                initialized;
    uint8_t             hit_count;
    uint8_t             stable_index;
    uint8_t             cooldown_frames;
    uint8_t             noise_floor;
    RL_MinervaKwsResult last_result;
} RL_MinervaKwsContext;

static RL_MinervaKwsContext s_rl_minerva_kws;
static TaskHandle_t s_rl_minerva_kws_task = NULL;
static int16_t s_rl_minerva_ring[RL_MINERVA_KWS_RING_SAMPLES];
static volatile uint16_t s_rl_minerva_head = 0U;
static volatile uint16_t s_rl_minerva_tail = 0U;
static volatile uint32_t s_rl_minerva_dropped = 0U;
static int16_t s_rl_minerva_frame[RL_MINERVA_KWS_FRAME_SAMPLES];
static int32_t s_rl_minerva_fft[RL_MINERVA_KWS_FFT_SIZE];
static uint32_t s_rl_minerva_mel[RL_MINERVA_KWS_MEL_BINS];
static mnv_act_t s_rl_minerva_input[RL_MINERVA_KWS_INPUT_FEATURES];
static mnv_act_t s_rl_minerva_output[MNV_OUTPUT_SIZE];
static mnv_act_t s_rl_minerva_output_smooth[MNV_OUTPUT_SIZE];

static int32_t rl_minerva_kws_abs32(int32_t v)
{
    return (v < 0) ? -v : v;
}

static int16_t rl_minerva_kws_pcm_to_s16(const PCM_DATA_TYPE *pcm, uint32_t index, uint8_t width)
{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
    int32_t v = pcm[index];

    if ((width == WIDTH_L24BIT) || (width == WIDTH_24BIT) || (width == WIDTH_H24BIT)) {
        return (int16_t)(v >> 8);
    }

    return (int16_t)v;
#else
    (void)width;
    return (int16_t)pcm[index];
#endif
}

static uint16_t rl_minerva_kws_ring_count(void)
{
    uint16_t head = s_rl_minerva_head;
    uint16_t tail = s_rl_minerva_tail;

    if (head >= tail) {
        return (uint16_t)(head - tail);
    }

    return (uint16_t)(RL_MINERVA_KWS_RING_SAMPLES - tail + head);
}

static bool rl_minerva_kws_ring_pop_frame(int16_t *out)
{
    uint16_t tail;
    uint16_t i;

    if (rl_minerva_kws_ring_count() < RL_MINERVA_KWS_FRAME_SAMPLES) {
        return false;
    }

    tail = s_rl_minerva_tail;
    for (i = 0; i < RL_MINERVA_KWS_FRAME_SAMPLES; i++) {
        out[i] = s_rl_minerva_ring[tail];
        tail++;
        if (tail >= RL_MINERVA_KWS_RING_SAMPLES) {
            tail = 0U;
        }
    }
    s_rl_minerva_tail = tail;
    return true;
}

static uint8_t rl_minerva_kws_log2_u32(uint32_t v)
{
    uint8_t r = 0U;

    while (v > 1U) {
        v >>= 1;
        r++;
    }

    return r;
}

static int16_t rl_minerva_kws_clamp_s8(int32_t v)
{
    if (v > 127) {
        return 127;
    }
    if (v < -128) {
        return -128;
    }
    return (int16_t)v;
}

static uint8_t rl_minerva_kws_frame_energy_level(const int16_t *frame)
{
    uint32_t acc = 0U;
    uint16_t i;

    for (i = 0U; i < RL_MINERVA_KWS_FRAME_SAMPLES; i += RL_MINERVA_KWS_DECIM_STRIDE) {
        acc += (uint32_t)rl_minerva_kws_abs32(frame[i]);
    }

    return (uint8_t)((acc / (RL_MINERVA_KWS_FRAME_SAMPLES / RL_MINERVA_KWS_DECIM_STRIDE)) >> 8);
}

static void rl_minerva_kws_extract_features(const int16_t *frame)
{
    uint16_t i;
    uint16_t bin;
    uint32_t energy_acc = 0U;
    uint32_t zcr = 0U;
    uint32_t peak = 0U;
    uint32_t low_energy = 0U;
    uint32_t mid_energy = 0U;
    uint32_t high_energy = 0U;
    uint32_t flux = 0U;
    uint32_t centroid_num = 0U;
    uint32_t centroid_den = 0U;
    uint32_t rolloff_den = 0U;
    uint32_t rolloff_acc = 0U;
    uint16_t rolloff_bin = 0U;
    uint16_t band;
    int16_t prev = frame[0];

    memset(s_rl_minerva_fft, 0, sizeof(s_rl_minerva_fft));
    memset(s_rl_minerva_mel, 0, sizeof(s_rl_minerva_mel));
    memset(s_rl_minerva_input, 0, sizeof(s_rl_minerva_input));

    for (i = 0; i < (RL_MINERVA_KWS_FRAME_SAMPLES / RL_MINERVA_KWS_DECIM_STRIDE); i++) {
        uint16_t idx = (uint16_t)(i * RL_MINERVA_KWS_DECIM_STRIDE);
        int32_t sample = frame[idx];
        s_rl_minerva_fft[i] = sample;
        energy_acc += (uint32_t)rl_minerva_kws_abs32(sample);
        if ((uint32_t)rl_minerva_kws_abs32(sample) > peak) {
            peak = (uint32_t)rl_minerva_kws_abs32(sample);
        }
        if (((prev < 0) && (sample >= 0)) || ((prev >= 0) && (sample < 0))) {
            zcr++;
        }
        prev = (int16_t)sample;
    }

    if (rfft_api(s_rl_minerva_fft, RL_MINERVA_KWS_FFT_SIZE, 0) == 0) {
        return;
    }

    for (band = 0; band < RL_MINERVA_KWS_MEL_BINS; band++) {
        uint16_t start = s_rl_minerva_mel_edges[band];
        uint16_t center = s_rl_minerva_mel_edges[band + 1U];
        uint16_t end = s_rl_minerva_mel_edges[band + 2U];

        for (bin = start; bin < end; bin++) {
            uint32_t mag;
            uint32_t weight;

            if (bin == (RL_MINERVA_KWS_FFT_SIZE / 2U)) {
                mag = (uint32_t)rl_minerva_kws_abs32(s_rl_minerva_fft[1]);
            } else {
                mag = (uint32_t)(rl_minerva_kws_abs32(s_rl_minerva_fft[2U * bin]) +
                                 rl_minerva_kws_abs32(s_rl_minerva_fft[2U * bin + 1U]));
            }

            if (bin < center) {
                weight = (uint32_t)(bin - start + 1U);
            } else {
                weight = (uint32_t)(end - bin);
            }
            s_rl_minerva_mel[band] += mag * weight;
        }
    }

    for (i = 0U; i < RL_MINERVA_KWS_MFCC_COUNT; i++) {
        int32_t acc = 0;

        for (band = 0; band < RL_MINERVA_KWS_MEL_BINS; band++) {
            uint8_t logv = rl_minerva_kws_log2_u32((s_rl_minerva_mel[band] >> 12) + 1U);
            acc += (int32_t)s_rl_minerva_dct_q8[i][band] * (int32_t)logv;
        }

        s_rl_minerva_input[i] = (mnv_act_t)rl_minerva_kws_clamp_s8(acc >> 6);
    }

    for (bin = 1U; bin <= (RL_MINERVA_KWS_FFT_SIZE / 2U); bin++) {
        uint32_t mag;

        if (bin == (RL_MINERVA_KWS_FFT_SIZE / 2U)) {
            mag = (uint32_t)rl_minerva_kws_abs32(s_rl_minerva_fft[1]);
        } else {
            mag = (uint32_t)(rl_minerva_kws_abs32(s_rl_minerva_fft[2U * bin]) +
                             rl_minerva_kws_abs32(s_rl_minerva_fft[2U * bin + 1U]));
        }

        centroid_num += (uint32_t)bin * (mag >> 8);
        centroid_den += (mag >> 8);
        rolloff_den += (mag >> 8);

        if (bin < 32U) {
            low_energy += (mag >> 8);
        } else if (bin < 96U) {
            mid_energy += (mag >> 8);
        } else {
            high_energy += (mag >> 8);
        }

        if (bin > 1U) {
            uint32_t prev_mag;
            if ((bin - 1U) == (RL_MINERVA_KWS_FFT_SIZE / 2U)) {
                prev_mag = (uint32_t)rl_minerva_kws_abs32(s_rl_minerva_fft[1]);
            } else {
                prev_mag = (uint32_t)(rl_minerva_kws_abs32(s_rl_minerva_fft[2U * (bin - 1U)]) +
                                      rl_minerva_kws_abs32(s_rl_minerva_fft[2U * (bin - 1U) + 1U]));
            }
            flux += (uint32_t)rl_minerva_kws_abs32((int32_t)(mag >> 8) - (int32_t)(prev_mag >> 8));
        }
    }

    for (bin = 1U; bin <= (RL_MINERVA_KWS_FFT_SIZE / 2U); bin++) {
        uint32_t mag;

        if (bin == (RL_MINERVA_KWS_FFT_SIZE / 2U)) {
            mag = (uint32_t)rl_minerva_kws_abs32(s_rl_minerva_fft[1]);
        } else {
            mag = (uint32_t)(rl_minerva_kws_abs32(s_rl_minerva_fft[2U * bin]) +
                             rl_minerva_kws_abs32(s_rl_minerva_fft[2U * bin + 1U]));
        }

        rolloff_acc += (mag >> 8);
        if ((rolloff_acc * 20U) >= (rolloff_den * 17U)) {
            rolloff_bin = bin;
            break;
        }
    }

    s_rl_minerva_input[12] = (mnv_act_t)rl_minerva_kws_clamp_s8((int32_t)(energy_acc / (RL_MINERVA_KWS_FRAME_SAMPLES / RL_MINERVA_KWS_DECIM_STRIDE)));
    s_rl_minerva_input[13] = (mnv_act_t)(zcr > 127U ? 127U : zcr);
    s_rl_minerva_input[14] = (mnv_act_t)rl_minerva_kws_clamp_s8((int32_t)(rl_minerva_kws_log2_u32(energy_acc + 1U) * 8U));
    s_rl_minerva_input[15] = (mnv_act_t)rl_minerva_kws_clamp_s8((int32_t)(peak >> 8));
    s_rl_minerva_input[16] = (mnv_act_t)rl_minerva_kws_clamp_s8((int32_t)rl_minerva_kws_log2_u32(low_energy + 1U) * 8 - 64);
    s_rl_minerva_input[17] = (mnv_act_t)rl_minerva_kws_clamp_s8((int32_t)rl_minerva_kws_log2_u32(mid_energy + 1U) * 8 - 64);
    s_rl_minerva_input[18] = (mnv_act_t)rl_minerva_kws_clamp_s8((int32_t)rl_minerva_kws_log2_u32(high_energy + 1U) * 8 - 64);
    s_rl_minerva_input[19] = (mnv_act_t)rl_minerva_kws_clamp_s8((int32_t)((centroid_den == 0U) ? 0U : ((centroid_num * 127U) / (centroid_den * (RL_MINERVA_KWS_FFT_SIZE / 2U)))));

    for (band = 0U; band < RL_MINERVA_KWS_MEL_SUMMARY; band++) {
        uint16_t src = (uint16_t)((uint32_t)band * RL_MINERVA_KWS_MEL_BINS / RL_MINERVA_KWS_MEL_SUMMARY);
        uint16_t src_next = (uint16_t)(((uint32_t)(band + 1U) * RL_MINERVA_KWS_MEL_BINS) / RL_MINERVA_KWS_MEL_SUMMARY);
        uint32_t acc = 0U;
        uint16_t count = 0U;
        uint8_t logv;

        for (i = src; i < src_next; i++) {
            acc += s_rl_minerva_mel[i];
            count++;
        }

        if (count == 0U) {
            count = 1U;
        }

        logv = rl_minerva_kws_log2_u32(((acc / count) >> 12) + 1U);
        s_rl_minerva_input[20U + band] = (mnv_act_t)((logv > 15U) ? 63 : ((int16_t)logv * 8 - 64));
    }
}

static void rl_minerva_kws_publish_unknown(void)
{
    s_rl_minerva_kws.last_result.state = RL_MINERVA_KWS_STATE_READY;
    s_rl_minerva_kws.last_result.keyword_index = 0xFFU;
    s_rl_minerva_kws.last_result.confidence = 0U;
    s_rl_minerva_kws.last_result.frame_count++;
    s_rl_minerva_kws.last_result.dropped_samples = s_rl_minerva_dropped;
}

static void rl_minerva_kws_publish_result(uint8_t keyword_index, uint8_t confidence)
{
    s_rl_minerva_kws.last_result.state = (confidence >= RL_MINERVA_KWS_TRIGGER_THRESHOLD) ?
                                         RL_MINERVA_KWS_STATE_TRIGGERED :
                                         RL_MINERVA_KWS_STATE_READY;
    s_rl_minerva_kws.last_result.keyword_index = keyword_index;
    s_rl_minerva_kws.last_result.confidence = confidence;
    s_rl_minerva_kws.last_result.frame_count++;
    s_rl_minerva_kws.last_result.dropped_samples = s_rl_minerva_dropped;
}

static void rl_minerva_kws_task(void *param)
{
    (void)param;

    while (1) {
        RL_MinervaKws_Process();
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}

bool RL_MinervaKws_Init(void)
{
    memset(&s_rl_minerva_kws, 0, sizeof(s_rl_minerva_kws));
    s_rl_minerva_head = 0U;
    s_rl_minerva_tail = 0U;
    s_rl_minerva_dropped = 0U;
    s_rl_minerva_kws.last_result.state = RL_MINERVA_KWS_STATE_IDLE;

    if (!RL_Minerva_Init()) {
        APP_DBG("minerva kws: init failed\r\n");
        return false;
    }

    RL_Minerva_Seed(0x13572468UL);
    s_rl_minerva_kws.initialized = true;
    s_rl_minerva_kws.last_result.state = RL_MINERVA_KWS_STATE_READY;

    if (s_rl_minerva_kws_task == NULL) {
        if (xTaskCreate(rl_minerva_kws_task,
                        "minerva kws",
                        RL_MINERVA_KWS_TASK_STACK_SIZE,
                        NULL,
                        RL_MINERVA_KWS_TASK_PRIO,
                        &s_rl_minerva_kws_task) != pdTRUE) {
            APP_DBG("minerva kws: task fail\r\n");
            s_rl_minerva_kws.initialized = false;
            s_rl_minerva_kws_task = NULL;
            return false;
        }
    }

    return true;
}

void RL_MinervaKws_Reset(void)
{
    s_rl_minerva_head = 0U;
    s_rl_minerva_tail = 0U;
    s_rl_minerva_dropped = 0U;
    memset(s_rl_minerva_ring, 0, sizeof(s_rl_minerva_ring));
    memset(s_rl_minerva_frame, 0, sizeof(s_rl_minerva_frame));
    memset(s_rl_minerva_fft, 0, sizeof(s_rl_minerva_fft));
    memset(s_rl_minerva_input, 0, sizeof(s_rl_minerva_input));
    memset(s_rl_minerva_output, 0, sizeof(s_rl_minerva_output));
    memset(s_rl_minerva_output_smooth, 0, sizeof(s_rl_minerva_output_smooth));
    s_rl_minerva_kws.hit_count = 0U;
    s_rl_minerva_kws.stable_index = 0xFFU;
    s_rl_minerva_kws.cooldown_frames = 0U;
    s_rl_minerva_kws.noise_floor = RL_MINERVA_KWS_SILENCE_BASELINE;
    s_rl_minerva_kws.last_result.state = s_rl_minerva_kws.initialized ? RL_MINERVA_KWS_STATE_READY :
                                                                      RL_MINERVA_KWS_STATE_IDLE;
    s_rl_minerva_kws.last_result.keyword_index = 0xFFU;
    s_rl_minerva_kws.last_result.confidence = 0U;
    s_rl_minerva_kws.last_result.dropped_samples = 0U;
}

void RL_MinervaKws_AudioFramePush(const PCM_DATA_TYPE *pcm,
                                  uint16_t             samples,
                                  uint8_t              channels,
                                  uint8_t              width)
{
    uint16_t head;
    uint16_t i;

    if ((!s_rl_minerva_kws.initialized) || (pcm == NULL) || (samples == 0U)) {
        if ((pcm != NULL) && (samples != 0U) && !s_rl_minerva_kws.initialized) {
            if (!RL_MinervaKws_Init()) {
                return;
            }
        } else {
            return;
        }
    }

    if ((pcm == NULL) || (samples == 0U)) {
        return;
    }

    if (channels == 0U) {
        channels = 2U;
    }

    head = s_rl_minerva_head;
    for (i = 0; i < samples; i++) {
        uint16_t next = (uint16_t)(head + 1U);
        int16_t mono;

        if (next >= RL_MINERVA_KWS_RING_SAMPLES) {
            next = 0U;
        }

        if (next == s_rl_minerva_tail) {
            s_rl_minerva_dropped += (uint32_t)(samples - i);
            break;
        }

        if (channels >= 2U) {
            int16_t left = rl_minerva_kws_pcm_to_s16(pcm, (uint32_t)i * channels, width);
            int16_t right = rl_minerva_kws_pcm_to_s16(pcm, (uint32_t)i * channels + 1U, width);
            mono = (int16_t)(((int32_t)left + (int32_t)right) / 2);
        } else {
            mono = rl_minerva_kws_pcm_to_s16(pcm, i, width);
        }

        s_rl_minerva_ring[head] = mono;
        head = next;
    }

    s_rl_minerva_head = head;
}

void RL_MinervaKws_Process(void)
{
    uint8_t best_index;
    uint8_t confidence;
    uint8_t frame_energy;
    uint8_t i;

    if (!s_rl_minerva_kws.initialized) {
        return;
    }

    if (!rl_minerva_kws_ring_pop_frame(s_rl_minerva_frame)) {
        return;
    }

    frame_energy = rl_minerva_kws_frame_energy_level(s_rl_minerva_frame);
    if (frame_energy <= (uint8_t)(s_rl_minerva_kws.noise_floor + RL_MINERVA_KWS_SILENCE_MARGIN)) {
        s_rl_minerva_kws.noise_floor = (uint8_t)(((uint16_t)s_rl_minerva_kws.noise_floor * 7U + frame_energy) / 8U);
        s_rl_minerva_kws.hit_count = 0U;
        s_rl_minerva_kws.stable_index = 0xFFU;
        rl_minerva_kws_publish_unknown();
        return;
    }

    rl_minerva_kws_extract_features(s_rl_minerva_frame);

    if (RL_Minerva_Run(s_rl_minerva_input, MNV_INPUT_SIZE, s_rl_minerva_output, MNV_OUTPUT_SIZE) != MNV_OK) {
        s_rl_minerva_kws.hit_count = 0U;
        s_rl_minerva_kws.stable_index = 0xFFU;
        rl_minerva_kws_publish_unknown();
        return;
    }

    for (i = 0U; i < MNV_OUTPUT_SIZE; i++) {
        int16_t smooth = (int16_t)s_rl_minerva_output_smooth[i];
        int16_t cur = (int16_t)s_rl_minerva_output[i];
        s_rl_minerva_output_smooth[i] = (mnv_act_t)rl_minerva_kws_clamp_s8(
            ((int32_t)smooth * ((1U << RL_MINERVA_KWS_SMOOTH_SHIFT) - 1U) + cur) >> RL_MINERVA_KWS_SMOOTH_SHIFT
        );
    }

    best_index = mnv_ct_argmax(s_rl_minerva_output_smooth, MNV_OUTPUT_SIZE);
    confidence = (uint8_t)((int16_t)s_rl_minerva_output_smooth[best_index] + 128);

    if (s_rl_minerva_kws.cooldown_frames > 0U) {
        s_rl_minerva_kws.cooldown_frames--;
        rl_minerva_kws_publish_unknown();
        return;
    }

    if (best_index == s_rl_minerva_kws.stable_index) {
        if (s_rl_minerva_kws.hit_count < 255U) {
            s_rl_minerva_kws.hit_count++;
        }
    } else {
        s_rl_minerva_kws.stable_index = best_index;
        s_rl_minerva_kws.hit_count = 1U;
    }

    if ((confidence >= RL_MINERVA_KWS_TRIGGER_THRESHOLD) &&
        (s_rl_minerva_kws.hit_count >= RL_MINERVA_KWS_HIT_COUNT)) {
        s_rl_minerva_kws.cooldown_frames = RL_MINERVA_KWS_COOLDOWN_FRAMES;
        rl_minerva_kws_publish_result(best_index, confidence);
        s_rl_minerva_kws.hit_count = 0U;
        s_rl_minerva_kws.stable_index = 0xFFU;
    } else {
        rl_minerva_kws_publish_unknown();
    }
}

RL_MinervaKwsResult RL_MinervaKws_GetResult(void)
{
    RL_MinervaKwsResult result = s_rl_minerva_kws.last_result;
    result.dropped_samples = s_rl_minerva_dropped;
    return result;
}
