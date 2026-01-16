#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../include/multi_gnss_sim.h"

static satellite_t satellites[MAX_SAT];
static int num_satellites = 0;
static rinex_nav_t nav_data[MAX_SAT];

int multi_gnss_sim_init(gnss_config_t *config)
{
    if (config->verbose) {
        printf("Initializing Multi-GNSS SDR Simulator...\n");
    }
    
    // Load RINEX navigation data
    if (strlen(config->nav_file) > 0) {
        if (parse_rinex_nav(config->nav_file, nav_data, &num_satellites) != 0) {
            fprintf(stderr, "Error: Failed to parse RINEX navigation file\n");
            return -1;
        }
        
        if (config->verbose) {
            printf("Loaded %d satellites from RINEX file\n", num_satellites);
        }
    }
    
    // Initialize satellites based on enabled constellations
    num_satellites = 0;
    
    for (int i = 0; i < MAX_CONSTELLATIONS; i++) {
        if (!config->constellations[i]) continue;
        
        switch (i) {
            case CONSTELLATION_GPS:
                gps_init_satellites(satellites, &num_satellites);
                break;
            case CONSTELLATION_GLONASS:
                glonass_init_satellites(satellites, &num_satellites);
                break;
            case CONSTELLATION_GALILEO:
                galileo_init_satellites(satellites, &num_satellites);
                break;
            case CONSTELLATION_BEIDOU:
                beidou_init_satellites(satellites, &num_satellites);
                break;
        }
    }
    
    if (config->verbose) {
        printf("Initialized %d satellites\n", num_satellites);
    }
    
    // Load user motion if dynamic mode
    if (config->dynamic_mode && strlen(config->motion_file) > 0) {
        user_position_t motion[USER_MOTION_SIZE];
        int motion_count = 0;
        
        if (load_user_motion(config->motion_file, motion, &motion_count) != 0) {
            fprintf(stderr, "Error: Failed to load user motion file\n");
            return -1;
        }
        
        if (config->verbose) {
            printf("Loaded %d motion samples\n", motion_count);
        }
    }
    
    return 0;
}

int multi_gnss_sim_run(gnss_config_t *config)
{
    if (config->verbose) {
        printf("Starting simulation...\n");
        printf("Duration: %.1f seconds\n", config->duration);
        printf("Sample rate: %.0f Hz\n", config->sample_rate);
        printf("Output file: %s\n", config->output_file);
    }
    
    // Calculate total samples
    int total_samples = (int)(config->duration * config->sample_rate);
    int samples_per_ms = (int)(config->sample_rate / 1000.0);
    
    // Allocate IQ buffer
    double *iq_buffer = malloc(total_samples * 2 * sizeof(double));
    if (!iq_buffer) {
        fprintf(stderr, "Error: Failed to allocate IQ buffer\n");
        return -1;
    }
    
    // Initialize current user position
    user_position_t current_pos = config->user_pos;
    double current_time = config->start_time;
    
    // Main simulation loop
    for (int sample_idx = 0; sample_idx < total_samples; sample_idx += samples_per_ms) {
        double elapsed_time = (double)sample_idx / config->sample_rate;
        double sim_time = current_time + elapsed_time;
        
        // Update user position if dynamic
        if (config->dynamic_mode) {
            // Interpolate position for current time
            // update_user_position(&current_pos, elapsed_time);
        }
        
        // Clear IQ buffer for this millisecond
        memset(iq_buffer + sample_idx * 2, 0, samples_per_ms * 2 * sizeof(double));
        
        // Process each satellite
        for (int sat_idx = 0; sat_idx < num_satellites; sat_idx++) {
            satellite_t *sat = &satellites[sat_idx];
            
            // Update satellite position and time
            double sat_time = sim_time;
            switch (sat->constellation) {
                case CONSTELLATION_GPS:
                    // Update GPS satellite position
                    break;
                case CONSTELLATION_GLONASS:
                    sat_time = glonass_time_from_gps(sim_time);
                    break;
                case CONSTELLATION_GALILEO:
                    sat_time = galileo_time_from_gps(sim_time);
                    break;
                case CONSTELLATION_BEIDOU:
                    sat_time = beidou_time_from_gps(sim_time);
                    break;
            }
            
            // Generate signal for this satellite
            double sat_iq[samples_per_ms * 2];
            
            switch (sat->constellation) {
                case CONSTELLATION_GPS:
                    gps_generate_signal(sat, &current_pos, sat_iq, samples_per_ms);
                    break;
                case CONSTELLATION_GLONASS:
                    glonass_generate_signal(sat, &current_pos, sat_iq, samples_per_ms);
                    break;
                case CONSTELLATION_GALILEO:
                    galileo_generate_signal(sat, &current_pos, sat_iq, samples_per_ms);
                    break;
                case CONSTELLATION_BEIDOU:
                    beidou_generate_signal(sat, &current_pos, sat_iq, samples_per_ms);
                    break;
            }
            
            // Add to main IQ buffer
            for (int i = 0; i < samples_per_ms * 2; i++) {
                iq_buffer[sample_idx * 2 + i] += sat_iq[i];
            }
        }
        
        // Progress indicator
        if (config->verbose && (sample_idx % (int)(config->sample_rate * 10)) == 0) {
            double progress = (double)sample_idx / total_samples * 100.0;
            printf("Progress: %.1f%%\n", progress);
        }
    }
    
    // Save or transmit IQ data
    int result = 0;
    switch (config->sdr_device) {
        case SDR_IQFILE:
            result = save_iq_file(config->output_file, iq_buffer, total_samples, config->iq_bits);
            break;
        case SDR_HACKRF:
            result = hackrf_transmit(iq_buffer, total_samples, config->freq_gps, config->tx_gain);
            break;
        case SDR_PLUTO:
            result = pluto_transmit(iq_buffer, total_samples, config->freq_gps, config->tx_gain);
            break;
        case SDR_BLADERF:
            result = bladerf_transmit(iq_buffer, total_samples, config->freq_gps, config->tx_gain);
            break;
        case SDR_USRP:
            result = usrp_transmit(iq_buffer, total_samples, config->freq_gps, config->tx_gain);
            break;
        case SDR_LIMESDR:
            result = limesdr_transmit(iq_buffer, total_samples, config->freq_gps, config->tx_gain);
            break;
        default:
            fprintf(stderr, "Error: Unknown SDR device type\n");
            result = -1;
            break;
    }
    
    free(iq_buffer);
    
    if (config->verbose) {
        if (result == 0) {
            printf("Simulation completed successfully\n");
        } else {
            printf("Simulation completed with errors\n");
        }
    }
    
    return result;
}

void multi_gnss_sim_cleanup(gnss_config_t *config)
{
    if (config->verbose) {
        printf("Cleaning up simulator...\n");
    }
    
    // Clean up resources
    num_satellites = 0;
    memset(satellites, 0, sizeof(satellites));
    memset(nav_data, 0, sizeof(nav_data));
}