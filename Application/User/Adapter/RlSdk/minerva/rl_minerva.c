#include <string.h>

#include "rl_minerva.h"

__attribute__((weak)) const mnv_model_t mnv_model = {0};

static mnv_ctx_t s_rl_minerva_ctx;
static bool s_rl_minerva_initialized = false;

static bool rl_minerva_model_is_usable(void)
{
    return (mnv_model.version == MNV_ABI_VERSION) &&
           (mnv_model.key != NULL) &&
           (mnv_model.crypto != NULL) &&
           (mnv_model.encrypted_weights != NULL) &&
           (mnv_model.encrypted_len > 0U);
}

bool RL_Minerva_HasModel(void)
{
    return rl_minerva_model_is_usable();
}

bool RL_Minerva_Init(void)
{
    mnv_status_t status;

    if (s_rl_minerva_initialized) {
        return true;
    }

    if (!rl_minerva_model_is_usable()) {
        return false;
    }

    status = mnv_init(&s_rl_minerva_ctx, &mnv_model);
    if (status != MNV_OK) {
        mnv_destroy(&s_rl_minerva_ctx);
        return false;
    }

    s_rl_minerva_initialized = true;
    return true;
}

void RL_Minerva_Deinit(void)
{
    mnv_destroy(&s_rl_minerva_ctx);
    s_rl_minerva_initialized = false;
}

bool RL_Minerva_IsReady(void)
{
    return s_rl_minerva_initialized && rl_minerva_model_is_usable();
}

void RL_Minerva_Seed(uint32_t seed)
{
    if (!s_rl_minerva_initialized) {
        return;
    }

    mnv_seed_prng(&s_rl_minerva_ctx, seed);
}

bool RL_Minerva_RunSelfTest(void)
{
    mnv_act_t input[MNV_INPUT_SIZE] = {0};
    mnv_act_t output[MNV_OUTPUT_SIZE] = {0};

    if (!RL_Minerva_Init()) {
        return false;
    }

    RL_Minerva_Seed(0x12345678UL);
    return RL_Minerva_Run(input, MNV_INPUT_SIZE, output, MNV_OUTPUT_SIZE) == MNV_OK;
}

mnv_status_t RL_Minerva_Run(const mnv_act_t *input,
                            uint16_t         input_len,
                            mnv_act_t       *output,
                            uint16_t         output_len)
{
    if (!s_rl_minerva_initialized) {
        return MNV_ERR_CONFIG;
    }

    if ((input == NULL) || (output == NULL)) {
        return MNV_ERR_NULL;
    }

    if ((input_len != MNV_INPUT_SIZE) || (output_len != MNV_OUTPUT_SIZE)) {
        return MNV_ERR_CONFIG;
    }

    return mnv_run_with_model(&s_rl_minerva_ctx, &mnv_model, input, output);
}

mnv_status_t RL_Minerva_VerifyModel(void)
{
    if (!s_rl_minerva_initialized) {
        return MNV_ERR_CONFIG;
    }

    return mnv_verify(&s_rl_minerva_ctx, &mnv_model);
}

void RL_Minerva_GetOutputMac(uint8_t *mac, uint16_t mac_len)
{
    if ((mac == NULL) || (mac_len < MNV_OUTPUT_MAC_SIZE) || !s_rl_minerva_initialized) {
        return;
    }

    mnv_get_output_mac(&s_rl_minerva_ctx, mac);
}

const mnv_model_t *RL_Minerva_GetModel(void)
{
    return &mnv_model;
}

const mnv_ctx_t *RL_Minerva_GetContext(void)
{
    return s_rl_minerva_initialized ? &s_rl_minerva_ctx : NULL;
}
