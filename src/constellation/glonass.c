#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../include/multi_gnss_sim.h"

// GLONASS constants
#define GLONASS_CA_CODE_LENGTH 511
#define GLONASS_CODE_CHIPPING_RATE 0.511e6
#define GLONASS_L1_FREQ_BASE 1602.0e6
#define GLONASS_FREQ_STEP 562500.0

// GLONASS PRN to frequency channel mapping (can be configured)
static const int glonass_channels[24] = {
    1,  -4,  5,  6,  1, -4,  5,  6,  1, -4,  5,  6,
    1,  -4,  5,  6,  1, -4,  5,  6,  1, -4,  5,  6
};

// Generate GLONASS CA code for given PRN
void generate_glonass_code(int prn, int *code, int length)
{
    if (prn < 1 || prn > 24 || length != GLONASS_CA_CODE_LENGTH) {
        return;
    }
    
    // GLONASS uses maximum length sequences
    // Initialize 9-stage LFSR
    int state = 0b111111111;  // All ones
    
    for (int i = 0; i < GLONASS_CA_CODE_LENGTH; i++) {
        // Output is last bit
        int output = state & 1;
        code[i] = output ? -1 : 1;
        
        // Feedback from taps 9 and 10 (positions 8 and 9 in zero-based)
        int feedback = ((state >> 8) ^ (state >> 7)) & 1;
        
        // Shift register
        state = (state >> 1) | (feedback << 8);
    }
}

// Initialize GLONASS satellites
int glonass_init_satellites(satellite_t *sats, int *count)
{
    for (int prn = 1; prn <= 24; prn++) {
        satellite_t *sat = &sats[*count];
        
        sat->prn = prn;
        sat->constellation = CONSTELLATION_GLONASS;
        sat->available = true;
        sat->health = 0.0;
        
        // Generate CA code for this PRN
        int ca_code[GLONASS_CA_CODE_LENGTH];
        generate_glonass_code(prn, ca_code, GLONASS_CA_CODE_LENGTH);
        
        // Calculate L1 frequency for this satellite
        int channel = glonass_channels[prn - 1];
        sat->xyz[0] = sat->xyz[1] = sat->xyz[2] = 0.0;
        sat->vel[0] = sat->vel[1] = sat->vel[2] = 0.0;
        sat->clock_bias = 0.0;
        sat->clock_drift = 0.0;
        sat->clock_rate = 0.0;
        sat->iod = 0.0;
        
        (*count)++;
    }
    
    return 0;
}

// Generate GLONASS L1 C/A signal
int glonass_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count)
{
    if (!sat || !user || !iq_samples || count <= 0) {
        return -1;
    }
    
    // Generate CA code for this satellite
    int ca_code[GLONASS_CA_CODE_LENGTH];
    generate_glonass_code(sat->prn, ca_code, GLONASS_CA_CODE_LENGTH);
    
    // Calculate range and Doppler
    double range = compute_pseudorange(sat, user);
    double doppler = compute_doppler(sat, user);
    
    // Calculate carrier frequency with Doppler
    int channel = glonass_channels[sat->prn - 1];
    double carrier_freq = GLONASS_L1_FREQ_BASE + channel * GLONASS_FREQ_STEP + doppler;
    
    // Calculate code rate with Doppler
    double code_rate = GLONASS_CODE_CHIPPING_RATE * (1.0 + doppler / carrier_freq);
    
    // Generate samples
    for (int i = 0; i < count; i++) {
        double t = (double)i / 2600000.0;  // Assuming 2.6 MHz sample rate
        
        // Carrier phase
        double carrier_phase = 2.0 * PI * carrier_freq * t;
        
        // Code phase (accounting for range delay)
        double code_phase = fmod(code_rate * t - range * GLONASS_CODE_CHIPPING_RATE / 299792458.0, GLONASS_CA_CODE_LENGTH);
        
        if (code_phase < 0) {
            code_phase += GLONASS_CA_CODE_LENGTH;
        }
        
        // Get code chip value
        int chip_idx = (int)code_phase;
        int chip_value = ca_code[chip_idx];
        
        // Generate I and Q samples (BPSK modulation)
        double carrier = cos(carrier_phase);
        
        iq_samples[i * 2] = chip_value * carrier;      // I
        iq_samples[i * 2 + 1] = 0.0;                 // Q (BPSK has no Q component)
    }
    
    return 0;
}

// Update GLONASS satellite position from ephemeris
void glonass_update_satellite_position(satellite_t *sat, rinex_nav_t *nav, double glonass_time)
{
    if (!sat || !nav) return;
    
    // GLONASS uses different coordinate system and time system
    // This is a simplified implementation
    
    // GLONASS orbital period (seconds)
    double period = 43200.0;  // 12 hours
    
    // Time since ephemeris reference
    double dt = glonass_time - nav->toe;
    
    // Mean anomaly
    double M = 2.0 * PI * dt / period;
    
    // Simplified position calculation (circular orbit assumption)
    double radius = nav->sqrt_a * nav->sqrt_a;  // Semi-major axis squared
    
    // Position in orbital plane
    double x_orb = radius * cos(M);
    double y_orb = radius * sin(M);
    
    // Apply inclination rotation (GLONASS inclination ~64.8 degrees)
    double inc = 64.8 * PI / 180.0;
    double x_rot = x_orb;
    double y_rot = y_orb * cos(inc);
    double z_rot = y_orb * sin(inc);
    
    // Apply right ascension rotation
    double ra = nav->omega0;  // Right ascension
    sat->xyz[0] = x_rot * cos(ra) - y_rot * sin(ra);
    sat->xyz[1] = x_rot * sin(ra) + y_rot * cos(ra);
    sat->xyz[2] = z_rot;
    
    // Update clock correction
    double dt_clock = glonass_time - nav->toc;
    sat->clock_bias = nav->f0 + nav->f1 * dt_clock + nav->f2 * dt_clock * dt_clock;
    sat->clock_drift = nav->f1 + 2.0 * nav->f2 * dt_clock;
}