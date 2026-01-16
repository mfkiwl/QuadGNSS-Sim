#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "multi_gnss_sim.h"

// Save IQ data to file
int save_iq_file(const char *filename, double *iq_samples, int count, int bits)
{
    if (!filename || !iq_samples || count <= 0) {
        return -1;
    }
    
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open output file %s\n", filename);
        return -1;
    }
    
    printf("Saving %d IQ samples to %s (%d-bit)\n", count, filename, bits);
    
    switch (bits) {
        case 16: {
            // 16-bit signed samples
            int16_t *samples_16 = malloc(count * 2 * sizeof(int16_t));
            if (!samples_16) {
                fclose(fp);
                return -1;
            }
            
            // Scale and convert to 16-bit
            for (int i = 0; i < count * 2; i++) {
                double val = iq_samples[i] * 32767.0;  // Scale to full range
                if (val > 32767.0) val = 32767.0;
                if (val < -32768.0) val = -32768.0;
                samples_16[i] = (int16_t)val;
            }
            
            fwrite(samples_16, sizeof(int16_t), count * 2, fp);
            free(samples_16);
            break;
        }
        
        case 8: {
            // 8-bit signed samples
            int8_t *samples_8 = malloc(count * 2 * sizeof(int8_t));
            if (!samples_8) {
                fclose(fp);
                return -1;
            }
            
            // Scale and convert to 8-bit
            for (int i = 0; i < count * 2; i++) {
                double val = iq_samples[i] * 127.0;  // Scale to full range
                if (val > 127.0) val = 127.0;
                if (val < -128.0) val = -128.0;
                samples_8[i] = (int8_t)val;
            }
            
            fwrite(samples_8, sizeof(int8_t), count * 2, fp);
            free(samples_8);
            break;
        }
        
        case 1: {
            // 1-bit samples (packed)
            uint8_t *samples_1 = malloc((count * 2 + 7) / 8);
            if (!samples_1) {
                fclose(fp);
                return -1;
            }
            
            memset(samples_1, 0, (count * 2 + 7) / 8);
            
            for (int i = 0; i < count * 2; i++) {
                if (iq_samples[i] > 0) {
                    samples_1[i / 8] |= (1 << (7 - (i % 8)));
                }
            }
            
            fwrite(samples_1, sizeof(uint8_t), (count * 2 + 7) / 8, fp);
            free(samples_1);
            break;
        }
        
        default:
            fprintf(stderr, "Error: Unsupported IQ bit depth %d\n", bits);
            fclose(fp);
            return -1;
    }
    
    fclose(fp);
    printf("Successfully saved %d samples to %s\n", count, filename);
    return 0;
}

// Stub implementations for SDR interfaces (will be implemented later)
int hackrf_transmit(double *iq_samples, int count, double freq, double gain)
{
    printf("HackRF transmission not yet implemented\n");
    printf("Frequency: %.0f Hz, Gain: %.1f dB, Samples: %d\n", freq, gain, count);
    return 0;
}

int pluto_transmit(double *iq_samples, int count, double freq, double gain)
{
    printf("PlutoSDR transmission not yet implemented\n");
    printf("Frequency: %.0f Hz, Gain: %.1f dB, Samples: %d\n", freq, gain, count);
    return 0;
}

int bladerf_transmit(double *iq_samples, int count, double freq, double gain)
{
    printf("bladeRF transmission not yet implemented\n");
    printf("Frequency: %.0f Hz, Gain: %.1f dB, Samples: %d\n", freq, gain, count);
    return 0;
}

int usrp_transmit(double *iq_samples, int count, double freq, double gain)
{
    printf("USRP transmission not yet implemented\n");
    printf("Frequency: %.0f Hz, Gain: %.1f dB, Samples: %d\n", freq, gain, count);
    return 0;
}

int limesdr_transmit(double *iq_samples, int count, double freq, double gain)
{
    printf("LimeSDR transmission not yet implemented\n");
    printf("Frequency: %.0f Hz, Gain: %.1f dB, Samples: %d\n", freq, gain, count);
    return 0;
}