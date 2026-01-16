#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../include/multi_gnss_sim.h"

// Galileo E1 constants
#define GALILEO_E1_CODE_LENGTH 4092
#define GALILEO_CODE_CHIPPING_RATE 1.023e6
#define GALILEO_E1_FREQ 1575.42e6

// Generate Galileo E1 OS code for given PRN
void generate_e1_code(int prn, int *code, int length)
{
    if (prn < 1 || prn > 36 || length != GALILEO_E1_CODE_LENGTH) {
        return;
    }
    
    // Galileo uses tiered codes - simplified implementation
    // This is a basic code generator, actual Galileo codes are more complex
    
    // Initialize LFSR for primary code
    int state = prn;  // Use PRN as seed
    
    for (int i = 0; i < GALILEO_E1_CODE_LENGTH; i++) {
        // Generate pseudo-random chip
        int output = state & 1;
        code[i] = output ? -1 : 1;
        
        // Simple LFSR feedback
        int feedback = ((state >> 3) ^ (state >> 0)) & 1;
        state = (state >> 1) | (feedback << 15);
    }
}

// Initialize Galileo satellites
int galileo_init_satellites(satellite_t *sats, int *count)
{
    for (int prn = 1; prn <= 36; prn++) {
        satellite_t *sat = &sats[*count];
        
        sat->prn = prn;
        sat->constellation = CONSTELLATION_GALILEO;
        sat->available = true;
        sat->health = 0.0;
        
        // Generate E1 code for this PRN
        int e1_code[GALILEO_E1_CODE_LENGTH];
        generate_e1_code(prn, e1_code, GALILEO_E1_CODE_LENGTH);
        
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

// Generate Galileo E1 OS signal
int galileo_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count)
{
    if (!sat || !user || !iq_samples || count <= 0) {
        return -1;
    }
    
    // Generate E1 code for this satellite
    int e1_code[GALILEO_E1_CODE_LENGTH];
    generate_e1_code(sat->prn, e1_code, GALILEO_E1_CODE_LENGTH);
    
    // Calculate range and Doppler
    double range = compute_pseudorange(sat, user);
    double doppler = compute_doppler(sat, user);
    
    // Calculate carrier frequency with Doppler
    double carrier_freq = GALILEO_E1_FREQ + doppler;
    
    // Calculate code rate with Doppler
    double code_rate = GALILEO_CODE_CHIPPING_RATE * (1.0 + doppler / GALILEO_E1_FREQ);
    
    // Generate samples
    for (int i = 0; i < count; i++) {
        double t = (double)i / 2600000.0;  // Assuming 2.6 MHz sample rate
        
        // Carrier phase
        double carrier_phase = 2.0 * PI * carrier_freq * t;
        
        // Code phase (accounting for range delay)
        double code_phase = fmod(code_rate * t - range * GALILEO_CODE_CHIPPING_RATE / 299792458.0, GALILEO_E1_CODE_LENGTH);
        
        if (code_phase < 0) {
            code_phase += GALILEO_E1_CODE_LENGTH;
        }
        
        // Get code chip value
        int chip_idx = (int)code_phase;
        int chip_value = e1_code[chip_idx];
        
        // Generate I and Q samples (BOC modulation for Galileo E1)
        double subcarrier_phase = 2.0 * PI * 1.023e6 * t;  // 1.023 MHz subcarrier
        double subcarrier = cos(subcarrier_phase);  // BOC(1,1) subcarrier
        
        // BOC modulation: code × subcarrier × carrier
        double carrier = cos(carrier_phase);
        double modulation = chip_value * subcarrier;
        
        iq_samples[i * 2] = modulation * carrier;      // I
        iq_samples[i * 2 + 1] = 0.0;                 // Q (simplified)
    }
    
    return 0;
}

// Update Galileo satellite position from ephemeris
void galileo_update_satellite_position(satellite_t *sat, rinex_nav_t *nav, double galileo_time)
{
    if (!sat || !nav) return;
    
    // Similar to GPS but with Galileo-specific parameters
    // Calculate time since ephemeris reference
    double dt = galileo_time - nav->toe;
    
    // Mean motion (Galileo uses different Earth parameters)
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
    double dt_clock = galileo_time - nav->toc;
    sat->clock_bias = nav->f0 + nav->f1 * dt_clock + nav->f2 * dt_clock * dt_clock;
    sat->clock_drift = nav->f1 + 2.0 * nav->f2 * dt_clock;
}