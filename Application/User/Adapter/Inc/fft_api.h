#ifndef FFT_API_H
#define FFT_API_H

#include <stdint.h>

int rfft_api(int32_t *buffer, uint16_t fft_size, uint8_t inverse);

#endif /* FFT_API_H */
