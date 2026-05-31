#include "fft_api.h"

/*
 * Minimal placeholder real-FFT adapter.
 *
 * It preserves the input buffer contract expected by rl_minerva_kws.c well
 * enough to keep the capture/KWS pipeline buildable. This is not a production
 * spectral implementation.
 */
int rfft_api(int32_t *buffer, uint16_t fft_size, uint8_t inverse)
{
    uint16_t i;

    (void)inverse;

    if ((buffer == 0) || (fft_size < 4U))
    {
        return 0;
    }

    for (i = 2U; i < fft_size; i++)
    {
        buffer[i] = buffer[i - 2U];
    }

    return 1;
}
