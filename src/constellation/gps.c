#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../include/multi_gnss_sim.h"

// GPS CA code generator constants
#define GPS_CA_CODE_LENGTH 1023
#define GPS_CODE_CHIPPING_RATE 1.023e6
#define GPS_L1_FREQ 1575.42e6

// GPS PRN tap positions for LFSR
static const int gps_taps[32][2] = {
    {2, 6}, {3, 7}, {4, 8}, {5, 9}, {1, 9}, {2, 10}, {1, 8}, {2, 9},
    {3, 10}, {2, 3}, {3, 4}, {5, 6}, {6, 7}, {7, 8}, {8, 9}, {9, 10},
    {1, 4}, {2, 5}, {3, 6}, {4, 7}, {5, 8}, {6, 9}, {1, 3}, {4, 6},
    {5, 7}, {6, 8}, {7, 9}, {8, 10}, {1, 6}, {2, 7}, {3, 8}, {4, 9}
};

// Generate GPS CA code for given PRN
void generate_ca_code(int prn, int *code, int length)
{
    if (prn < 1 || prn > 32 || length != GPS_CA_CODE_LENGTH) {
        return;
    }
    
    // Initialize LFSR registers
    int g1[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    int g2[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    
    // G2 tap positions for this PRN
    int tap1 = gps_taps[prn - 1][0] - 1;
    int tap2 = gps_taps[prn - 1][1] - 1;
    
    for (int i = 0; i < GPS_CA_CODE_LENGTH; i++) {
        // Calculate output
        int g1_out = g1[9];
        int g2_out = g2[tap1] ^ g2[tap2];
        code[i] = g1_out ^ g2_out;
        
        // Shift registers
        int g1_feedback = g1[2] ^ g1[9];
        int g2_feedback = g2[1] ^ g2[2] ^ g2[5] ^ g2[7] ^ g2[8] ^ g2[9];
        
        for (int j = 9; j > 0; j--) {
            g1[j] = g1[j - 1];
            g2[j] = g2[j - 1];
        }
        
        g1[0] = g1_feedback;
        g2[0] = g2_feedback;
    }
    
    // Convert 0/1 to -1/+1
    for (int i = 0; i < GPS_CA_CODE_LENGTH; i++) {
        code[i] = code[i] ? -1 : 1;
    }
}

// Initialize GPS satellites
int gps_init_satellites(satellite_t *sats, int *count)
{
    for (int prn = 1; prn <= 32; prn++) {
        satellite_t *sat = &sats[*count];
        
        sat->prn = prn;
        sat->constellation = CONSTELLATION_GPS;
        sat->available = true;
        sat->health = 0.0;
        
        // Generate CA code for this PRN
        int ca_code[GPS_CA_CODE_LENGTH];
        generate_ca_code(prn, ca_code, GPS_CA_CODE_LENGTH);
        
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

// Generate GPS L1 C/A signal
int gps_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count)
{
    if (!sat || !user || !iq_samples || count <= 0) {
        return -1;
    }
    
    // Generate CA code for this satellite
    int ca_code[GPS_CA_CODE_LENGTH];
    generate_ca_code(sat->prn, ca_code, GPS_CA_CODE_LENGTH);
    
    // Calculate range and Doppler
    double range = compute_pseudorange(sat, user);
    double doppler = compute_doppler(sat, user);
    
    // Calculate carrier frequency with Doppler
    double carrier_freq = GPS_L1_FREQ + doppler;
    
    // Calculate code rate with Doppler
    double code_rate = GPS_CODE_CHIPPING_RATE * (1.0 + doppler / GPS_L1_FREQ);
    
    // Generate samples
    for (int i = 0; i < count; i++) {
        double t = (double)i / 2600000.0;  // Assuming 2.6 MHz sample rate
        
        // Carrier phase
        double carrier_phase = 2.0 * PI * carrier_freq * t;
        
        // Code phase (accounting for range delay)
        double code_phase = fmod(code_rate * t - range * GPS_CODE_CHIPPING_RATE / 299792458.0, GPS_CA_CODE_LENGTH);
        
        if (code_phase < 0) {
            code_phase += GPS_CA_CODE_LENGTH;
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

// Update GPS satellite position from ephemeris
void gps_update_satellite_position(satellite_t *sat, rinex_nav_t *nav, double gps_time)
{
    if (!sat || !nav) return;
    
    // Calculate time since ephemeris reference
    double dt = gps_time - nav->toe;
    
    // Mean motion
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
    double dt_clock = gps_time - nav->toc;
    sat->clock_bias = nav->f0 + nav->f1 * dt_clock + nav->f2 * dt_clock * dt_clock;
    sat->clock_drift = nav->f1 + 2.0 * nav->f2 * dt_clock;
}