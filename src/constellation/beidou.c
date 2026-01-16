#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../include/multi_gnss_sim.h"

// BeiDou B1I constants
#define BEIDOU_B1I_CODE_LENGTH 2046
#define BEIDOU_CODE_CHIPPING_RATE 2.046e6
#define BEIDOU_B1I_FREQ 1561.098e6

// Generate BeiDou B1I code for given PRN
void generate_b1i_code(int prn, int *code, int length)
{
    if (prn < 1 || prn > 37 || length != BEIDOU_B1I_CODE_LENGTH) {
        return;
    }
    
    // BeiDou uses specific coding sequences
    // This is a simplified implementation
    
    // Initialize LFSR with PRN-based seed
    int state = prn * 1103515245 + 12345;  // Linear congruential generator
    
    for (int i = 0; i < BEIDOU_B1I_CODE_LENGTH; i++) {
        // Generate pseudo-random chip
        int output = state & 1;
        code[i] = output ? -1 : 1;
        
        // Next state
        state = (state * 1103515245 + 12345) & 0x7fffffff;
    }
}

// Initialize BeiDou satellites
int beidou_init_satellites(satellite_t *sats, int *count)
{
    for (int prn = 1; prn <= 37; prn++) {
        satellite_t *sat = &sats[*count];
        
        sat->prn = prn;
        sat->constellation = CONSTELLATION_BEIDOU;
        sat->available = true;
        sat->health = 0.0;
        
        // Generate B1I code for this PRN
        int b1i_code[BEIDOU_B1I_CODE_LENGTH];
        generate_b1i_code(prn, b1i_code, BEIDOU_B1I_CODE_LENGTH);
        
        // Default values (will be updated from ephemeris)
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

// Generate BeiDou B1I signal
int beidou_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count)
{
    if (!sat || !user || !iq_samples || count <= 0) {
        return -1;
    }
    
    // Generate B1I code for this satellite
    int b1i_code[BEIDOU_B1I_CODE_LENGTH];
    generate_b1i_code(sat->prn, b1i_code, BEIDOU_B1I_CODE_LENGTH);
    
    // Calculate range and Doppler
    double range = compute_pseudorange(sat, user);
    double doppler = compute_doppler(sat, user);
    
    // Calculate carrier frequency with Doppler
    double carrier_freq = BEIDOU_B1I_FREQ + doppler;
    
    // Calculate code rate with Doppler
    double code_rate = BEIDOU_CODE_CHIPPING_RATE * (1.0 + doppler / BEIDOU_B1I_FREQ);
    
    // Generate samples
    for (int i = 0; i < count; i++) {
        double t = (double)i / 2600000.0;  // Assuming 2.6 MHz sample rate
        
        // Carrier phase
        double carrier_phase = 2.0 * PI * carrier_freq * t;
        
        // Code phase (accounting for range delay)
        double code_phase = fmod(code_rate * t - range * BEIDOU_CODE_CHIPPING_RATE / 299792458.0, BEIDOU_B1I_CODE_LENGTH);
        
        if (code_phase < 0) {
            code_phase += BEIDOU_B1I_CODE_LENGTH;
        }
        
        // Get code chip value
        int chip_idx = (int)code_phase;
        int chip_value = b1i_code[chip_idx];
        
        // Generate I and Q samples (BPSK modulation)
        double carrier = cos(carrier_phase);
        
        iq_samples[i * 2] = chip_value * carrier;      // I
        iq_samples[i * 2 + 1] = 0.0;                 // Q (BPSK has no Q component)
    }
    
    return 0;
}

// Update BeiDou satellite position from ephemeris
void beidou_update_satellite_position(satellite_t *sat, rinex_nav_t *nav, double beidou_time)
{
    if (!sat || !nav) return;
    
    // BeiDou uses similar orbital model to GPS but with different parameters
    // Calculate time since ephemeris reference
    double dt = beidou_time - nav->toe;
    
    // Mean motion (BeiDou has different gravitational constant)
    double n = sqrt(398600441800000.0 / pow(nav->sqrt_a, 6.0)) + nav->delta_n;
    
    // Mean anomaly
    double M = nav->m0 + n * dt;
    
    // Solve Kepler's equation for eccentric anomaly
    double E = M;
    double e = nav->e;
    for (int iter = 0; iter < 10; iter++) {
        double dE = (E - e * sin(E) - M) / (1 - e * cos(E));
        E -= dE;
        if (fabs(dE) < 1e-10) break;
    }
    
    // True anomaly
    double nu = 2.0 * atan2(sqrt(1.0 + e) * sin(E / 2.0), sqrt(1.0 - e) * cos(E / 2.0));
    
    // Radius
    double r = nav->sqrt_a * nav->sqrt_a * (1.0 - e * cos(E));
    
    // Position in orbital plane
    double x_orb = r * cos(nu);
    double y_orb = r * sin(nu);
    
    // Argument of latitude
    double u = nu + nav->omega;
    
    // Perturbations
    double cos_2u = cos(2.0 * u);
    double sin_2u = sin(2.0 * u);
    
    double dr = nav->crc * cos_2u + nav->crs * sin_2u;
    double du = nav->cuc * cos_2u + nav->cus * sin_2u;
    double di = nav->cic * cos_2u + nav->cis * sin_2u;
    
    r += dr;
    u += du;
    
    // Corrected inclination
    double i = nav->i0 + di + nav->idot * dt;
    
    // Position in orbital plane
    double x_orb_corr = r * cos(u);
    double y_orb_corr = r * sin(u);
    
    // Corrected longitude of ascending node
    double omega_corrected = nav->omega0 + (nav->omegadot - 7.2921151467e-5) * dt - 7.2921151467e-5 * nav->toe;
    
    // Transform to ECEF coordinates
    sat->xyz[0] = x_orb_corr * cos(omega_corrected) - y_orb_corr * cos(i) * sin(omega_corrected);
    sat->xyz[1] = x_orb_corr * sin(omega_corrected) + y_orb_corr * cos(i) * cos(omega_corrected);
    sat->xyz[2] = y_orb_corr * sin(i);
    
    // Update clock correction
    double dt_clock = beidou_time - nav->toc;
    sat->clock_bias = nav->f0 + nav->f1 * dt_clock + nav->f2 * dt_clock * dt_clock;
    sat->clock_drift = nav->f1 + 2.0 * nav->f2 * dt_clock;
}