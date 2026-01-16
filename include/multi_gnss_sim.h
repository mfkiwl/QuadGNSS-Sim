#ifndef MULTI_GNSS_SIM_H
#define MULTI_GNSS_SIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

// Constants
#define MAX_SAT 64
#define USER_MOTION_SIZE 10000
#define MAX_CONSTELLATIONS 4
#define MAX_FILENAME 256
#define PI 3.14159265358979323846

// GNSS Constellation Types
typedef enum {
    CONSTELLATION_GPS = 0,
    CONSTELLATION_GLONASS = 1,
    CONSTELLATION_GALILEO = 2,
    CONSTELLATION_BEIDOU = 3,
    CONSTELLATION_NONE = -1
} gnss_constellation_t;

// Signal Types
typedef enum {
    SIGNAL_GPS_L1_CA,
    SIGNAL_GLONASS_L1_CA,
    SIGNAL_GALILEO_E1_OS,
    SIGNAL_BEIDOU_B1I
} signal_type_t;

// SDR Device Types
typedef enum {
    SDR_NONE = 0,
    SDR_HACKRF,
    SDR_PLUTO,
    SDR_BLADERF,
    SDR_USRP,
    SDR_LIMESDR,
    SDR_IQFILE
} sdr_device_t;

// Satellite Structure
typedef struct {
    int prn;
    gnss_constellation_t constellation;
    double xyz[3];           // ECEF position (m)
    double vel[3];           // Velocity (m/s)
    double clock_bias;       // Clock bias (s)
    double clock_drift;      // Clock drift (s/s)
    double clock_rate;       // Clock rate (s/s²)
    double iod;              // Issue of data
    double health;           // Health status
    bool available;          // Satellite available
    double azimuth;          // Azimuth angle (rad)
    double elevation;        // Elevation angle (rad)
    double pseudorange;      // Pseudorange (m)
    double doppler;          // Doppler frequency (Hz)
    double carrier_phase;    // Carrier phase (cycles)
    double snr;              // Signal-to-noise ratio (dB)
} satellite_t;

// User Position Structure
typedef struct {
    double xyz[3];           // ECEF position (m)
    double llh[3];           // Latitude, longitude, height (deg, m)
    double vel[3];           // ECEF velocity (m/s)
    double time;             // GPS time (s)
} user_position_t;

// GNSS Simulation Configuration
typedef struct {
    // Navigation data
    char nav_file[MAX_FILENAME];
    char rinex_version[16];
    bool use_ftp;
    char ftp_url[MAX_FILENAME];
    
    // Constellation selection
    bool constellations[MAX_CONSTELLATIONS];
    int num_constellations;
    
    // Position and motion
    user_position_t user_pos;
    char motion_file[MAX_FILENAME];
    bool static_mode;
    bool dynamic_mode;
    
    // Timing
    double start_time;
    double duration;
    double oscillator_error_ppb;
    
    // SDR output
    sdr_device_t sdr_device;
    char output_file[MAX_FILENAME];
    int iq_bits;
    double tx_gain;
    double sample_rate;
    
    // Simulation parameters
    bool disable_ionospheric;
    bool disable_tropospheric;
    bool verbose;
    bool interactive_mode;
    bool coherent_mode;
    
    // Frequencies (Hz)
    double freq_gps;
    double freq_glonass;
    double freq_galileo;
    double freq_beidou;
} gnss_config_t;

// RINEX Navigation Data Structure
typedef struct {
    gnss_constellation_t constellation;
    int prn;
    double toc;              // Time of clock
    double f0, f1, f2;       // Clock parameters
    double iod;              // Issue of data
    double crs, crc;         // Orbital parameters
    double cuc, cus;
    double cic, cis;
    double toe;              // Time of ephemeris
    double sqrt_a;           // Square root of semi-major axis
    double e;                // Eccentricity
    double omega0;           // Longitude of ascending node
    double omega;            // Argument of perigee
    double i0;               // Inclination
    double omegadot;         // Rate of right ascension
    double idot;             // Rate of inclination angle
    double delta_n;          // Mean motion difference
    double m0;               // Mean anomaly at reference time
    double health;           // Satellite health
} rinex_nav_t;

// Function Prototypes

// Main simulation functions
int multi_gnss_sim_init(gnss_config_t *config);
int multi_gnss_sim_run(gnss_config_t *config);
void multi_gnss_sim_cleanup(gnss_config_t *config);

// Configuration functions
int parse_command_line(int argc, char *argv[], gnss_config_t *config);
void load_default_config(gnss_config_t *config);
void print_usage(void);
void print_version(void);

// Constellation functions
int gps_init_satellites(satellite_t *sats, int *count);
int glonass_init_satellites(satellite_t *sats, int *count);
int galileo_init_satellites(satellite_t *sats, int *count);
int beidou_init_satellites(satellite_t *sats, int *count);

int gps_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count);
int glonass_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count);
int galileo_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count);
int beidou_generate_signal(satellite_t *sat, user_position_t *user, double *iq_samples, int count);

// RINEX parsing functions
int parse_rinex_nav(const char *filename, rinex_nav_t *nav_data, int *count);
int parse_rinex_obs(const char *filename, user_position_t *obs_data, int *count);
int download_rinex_ftp(const char *url, const char *local_file);

// Trajectory and motion functions
int load_user_motion(const char *filename, user_position_t *motion, int *count);
int interpolate_user_position(user_position_t *result, user_position_t *motion, 
                              int count, double time);
void update_user_position(user_position_t *user, double dt);

// Signal processing functions
void generate_ca_code(int prn, int *code, int length);
void generate_glonass_code(int prn, int *code, int length);
void generate_e1_code(int prn, int *code, int length);
void generate_b1i_code(int prn, int *code, int length);

void modulate_bpsk(double *carrier, int *code, double *iq, int length);
void modulate_boc(double *carrier, int *code, double *iq, int length);

// SDR output functions
int hackrf_transmit(double *iq_samples, int count, double freq, double gain);
int pluto_transmit(double *iq_samples, int count, double freq, double gain);
int bladerf_transmit(double *iq_samples, int count, double freq, double gain);
int usrp_transmit(double *iq_samples, int count, double freq, double gain);
int limesdr_transmit(double *iq_samples, int count, double freq, double gain);
int save_iq_file(const char *filename, double *iq_samples, int count, int bits);

// Utility functions
void ecef_to_llh(double xyz[3], double llh[3]);
void llh_to_ecef(double llh[3], double xyz[3]);
void compute_satellite_azimuth_el(satellite_t *sat, user_position_t *user);
double compute_pseudorange(satellite_t *sat, user_position_t *user);
double compute_doppler(satellite_t *sat, user_position_t *user);
double compute_ionospheric_delay(user_position_t *user, satellite_t *sat, double freq);
double compute_tropospheric_delay(user_position_t *user, satellite_t *sat);

// Time conversion functions
double gps_time_from_utc(double utc_time);
double utc_time_from_gps(double gps_time);
double galileo_time_from_gps(double gps_time);
double glonass_time_from_gps(double gps_time);
double beidou_time_from_gps(double gps_time);

// Math utility functions
void sub_vect(double *y, const double *x1, const double *x2);
void add_vect(double *y, const double *x1, const double *x2);
double dot_prod(const double *x1, const double *x2);
void cross_prod(double *y, const double *x1, const double *x2);
double norm_vect(const double *x);
void normalize_vect(double *x);

// Lookup tables
extern int sinTable512[];
extern int cosTable512[];
extern double ant_pat_db[];

#endif // MULTI_GNSS_SIM_H