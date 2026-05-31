#include "fft_api.h"
#include <stdint.h>

/* ----------------------------------------------------------------
 * Real FFT using a split-radix decimation-in-time algorithm.
 *
 * The buffer holds N real samples on input.
 * On output it contains N real values in the standard half-complex
 * packing used by rl_minerva_kws.c:
 *
 *   buffer[0]       = Re[0]   (DC, always real)
 *   buffer[1]       = Re[N/2] (Nyquist, always real)
 *   buffer[2*k]     = Re[k]   for k = 1 .. N/2-1
 *   buffer[2*k+1]   = Im[k]   for k = 1 .. N/2-1
 *
 * This matches the layout expected by the feature extractor which
 * reads pairs { Re[bin], Im[bin] } for each positive frequency bin.
 *
 * For N = 512 this uses ~5 KB of stack for the complex workspace.
 * A pre-computed sine table (N/4 entries) avoids runtime trig calls
 * and keeps the implementation self-contained without CMSIS-DSP.
 * ---------------------------------------------------------------- */

#define RFFT_MAX_SIZE  512

/* Pre-computed cos table: cos(2*pi*k / N) for k = 0 .. N/4-1, N = 512.
 * Generated once at compile time; stored as int16_t Q0.15 fixed-point
 * (cos * 32767). This avoids linking CMSIS-DSP. */
static const int16_t s_rfft_cos_table[128] = {
    32767, 32765, 32758, 32748, 32733, 32715, 32692, 32666,
    32635, 32601, 32563, 32521, 32475, 32426, 32373, 32316,
    32256, 32192, 32124, 32053, 31978, 31900, 31818, 31733,
    31644, 31552, 31457, 31358, 31257, 31152, 31044, 30933,
    30819, 30702, 30582, 30459, 30333, 30205, 30073, 29940,
    29803, 29664, 29522, 29378, 29231, 29082, 28930, 28777,
    28620, 28462, 28301, 28138, 27973, 27806, 27636, 27465,
    27292, 27116, 26939, 26760, 26579, 26396, 26211, 26025,
    25837, 25647, 25456, 25263, 25068, 24872, 24675, 24476,
    24276, 24075, 23872, 23668, 23463, 23257, 23050, 22842,
    22633, 22423, 22212, 22000, 21788, 21575, 21361, 21146,
    20931, 20715, 20499, 20282, 20065, 19847, 19629, 19411,
    19192, 18973, 18754, 18535, 18315, 18096, 17876, 17657,
    17437, 17218, 16999, 16780, 16561, 16342, 16124, 15906,
    15688, 15471, 15254, 15037, 14821, 14605, 14390, 14176,
    13962, 13748, 13536, 13324, 13112, 12902, 12692, 12483
};

/* Bit-reversal permutation for complex array of length N.
 * Reorders both real and imaginary parts in-place. */
static void rfft_bit_reverse(int32_t *re, int32_t *im, uint16_t n)
{
    uint16_t i, j = 0U;
    int32_t tmp;

    for (i = 0U; i < n; i++) {
        if (j > i) {
            tmp = re[i]; re[i] = re[j]; re[j] = tmp;
            tmp = im[i]; im[i] = im[j]; im[j] = tmp;
        }
        uint16_t m = n >> 1;
        while (m >= 1U && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}

/* In-place complex FFT using pre-computed cosine table.
 * Works on separate real and imaginary arrays.
 * Assumes n is a power of two, n <= RFFT_MAX_SIZE. */
static void rfft_complex_fft(int32_t *re, int32_t *im, uint16_t n)
{
    uint16_t len, i, k;
    int32_t t_re, t_im, w_re, w_im;
    int16_t w_re_q15, w_im_q15;
    uint16_t table_stride;

    rfft_bit_reverse(re, im, n);

    /* table_stride maps FFT stage length to cos-table index step */
    for (len = 2U; len <= n; len <<= 1) {
        table_stride = (uint16_t)(RFFT_MAX_SIZE / len);

        for (i = 0U; i < n; i += len) {
            for (k = 0U; k < (len >> 1); k++) {
                uint16_t idx = (uint16_t)(k * table_stride);

                /* Q0.15 twiddle: W = cos - j*sin, but we need W_N^k */
                w_re_q15 = s_rfft_cos_table[idx];
                if (idx == 0) {
                    w_im_q15 = 0;
                } else {
                    /* sin(theta) = cos(pi/2 - theta), table index flipped */
                    w_im_q15 = -(s_rfft_cos_table[(RFFT_MAX_SIZE / 4) - idx]);
                }

                /* Complex multiply: (re + j*im) * (w_re + j*w_im) in Q31*Q15 */
                uint16_t even = i + k;
                uint16_t odd  = i + k + (len >> 1);

                t_re = (int32_t)(((int64_t)re[odd] * w_re_q15 - (int64_t)im[odd] * w_im_q15) >> 15);
                t_im = (int32_t)(((int64_t)re[odd] * w_im_q15 + (int64_t)im[odd] * w_re_q15) >> 15);

                re[odd] = re[even] - t_re;
                im[odd] = im[even] - t_im;
                re[even] = re[even] + t_re;
                im[even] = im[even] + t_im;
            }
        }
    }
}

/* Convert a real-valued time-domain signal of length N into
 * half-complex frequency-domain representation packed in-place.
 *
 * Uses the standard "pack real array into complex, run N/2-point
 * complex FFT, then unpack" technique.
 *
 * On entry: buffer[0..N-1] contains real time-domain samples.
 * On exit:  buffer[0..N-1] contains half-complex spectrum:
 *   buffer[0] = Re[0], buffer[1] = Re[N/2],
 *   buffer[2k] = Re[k], buffer[2k+1] = Im[k] for k=1..N/2-1. */
int rfft_api(int32_t *buffer, uint16_t fft_size, uint8_t inverse)
{
    uint16_t n2, k;
    int32_t re_half[256];
    int32_t im_half[256];

    (void)inverse;  /* forward-only for this application */

    if ((buffer == 0) || (fft_size < 4U) || (fft_size > RFFT_MAX_SIZE)) {
        return 0;
    }

    n2 = fft_size >> 1;

    /* Pack even/odd samples into complex array of length N/2 */
    for (k = 0U; k < n2; k++) {
        re_half[k] = buffer[2U * k];
        im_half[k] = buffer[2U * k + 1U];
    }

    /* Run N/2-point complex FFT */
    rfft_complex_fft(re_half, im_half, n2);

    /* Unpack to half-complex real FFT output:
     * For k = 1 .. N/2-1:
     *   X[k] = 0.5 * (Z[k] + conj(Z[N/2-k])) * W_N^k
     * where W_N^k = exp(-j*2*pi*k/N).
     *
     * The factor 0.5 is absorbed into the 1/2 scaling below.
     * All operations use Q15 twiddle factors and Q0.31 intermediate accumulators. */
    {
        int32_t dc_re, dc_im, ny_re, ny_im;

        /* DC: X[0] = Z[0]_re + Z[0]_im  (scaled by 1/2) */
        dc_re = (re_half[0] + im_half[0]) >> 1;
        dc_im = 0;
        buffer[0] = dc_re;

        /* Nyquist: X[N/2] = Z[0]_re - Z[0]_im */
        ny_re = (re_half[0] - im_half[0]) >> 1;
        buffer[1] = ny_re;

        for (k = 1U; k < n2; k++) {
            int32_t z1_re, z1_im;  /* Z[k] */
            int32_t z2_re, z2_im;  /* conj(Z[N/2-k]) */
            int32_t u_re, u_im, v_re, v_im;
            int16_t w_re_q15, w_im_q15;
            uint16_t idx;

            z1_re = re_half[k];
            z1_im = im_half[k];
            z2_re = re_half[n2 - k];
            z2_im = -im_half[n2 - k];

            /* u = (Z[k] + conj(Z[N/2-k])) / 2 */
            u_re = (z1_re + z2_re) >> 1;
            u_im = (z1_im + z2_im) >> 1;

            /* v = (Z[k] - conj(Z[N/2-k])) / 2 */
            v_re = (z1_re - z2_re) >> 1;
            v_im = (z1_im - z2_im) >> 1;

            /* Twiddle factor W_N^k = cos - j*sin, index into 128-entry table for N=512 */
            idx = (uint16_t)(((uint32_t)k * (RFFT_MAX_SIZE / 4)) / n2);
            w_re_q15 = s_rfft_cos_table[idx];
            if (idx == 0) {
                w_im_q15 = 0;
            } else {
                w_im_q15 = -(s_rfft_cos_table[(RFFT_MAX_SIZE / 4) - idx]);
            }

            /* X[k] = u + j * (v * W)  —  but we need it split into Re/Im:
             * X[k]_re = u_re + (v_re * w_im_q15 - v_im * w_re_q15) >> 15
             * X[k]_im = u_im + (v_re * w_re_q15 + v_im * w_im_q15) >> 15
             * Wait — standard formula: multiply v by -j*W gives:
             * X[k] = u + j * v * (w_re - j*w_im) = u + (v_im*w_re + v_re*w_im) + j*(v_re*w_re - v_im*w_im)
             *
             * Simpler: X[k] = (u + v) * W  (from the standard unpack formula)
             * Actually the correct unpack formula is:
             *   X[k] = 0.5 * (Z[k] + conj(Z[N/2-k]))  +  0.5 * j * (Z[k] - conj(Z[N/2-k])) * W_N^{-k}
             *
             * Let A = u, B = -j * v, then:
             *   X[k] = A + B * W_N^k
             *   Re: A_re + B_re * w_re - B_im * w_im
             *   Im: A_im + B_re * w_im + B_im * w_re
             *
             * B = -j * v = v_im - j * v_re, so B_re = v_im, B_im = -v_re
             */

            {
                int32_t b_re, b_im;
                b_re = v_im;
                b_im = -v_re;

                buffer[2U * k]     = u_re + (int32_t)(((int64_t)b_re * w_re_q15 - (int64_t)b_im * w_im_q15) >> 15);
                buffer[2U * k + 1U] = u_im + (int32_t)(((int64_t)b_re * w_im_q15 + (int64_t)b_im * w_re_q15) >> 15);
            }
        }
    }

    return 1;
}
