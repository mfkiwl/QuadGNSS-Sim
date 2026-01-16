#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "multi_gnss_sim.h"

// Print usage information
void print_usage(void)
{
    printf("Multi-GNSS SDR Simulator v1.0.0\n");
    printf("Usage: multi-gnss-sdr-sim [options]\n\n");
    printf("Basic Options:\n");
    printf("  -h, --help           Show this help message\n");
    printf("  -V, --version        Show version information\n");
    printf("  -v, --verbose        Enable verbose output\n");
    printf("  -e <file>           RINEX navigation file\n");
    printf("  -l <lat,lon,hgt>    Static location (degrees, meters)\n");
    printf("  -c <list>           GNSS constellations (gps,glonass,galileo,beidou,all)\n");
    printf("  -d <seconds>         Simulation duration\n");
    printf("  -o <filename>        Output IQ file\n");
    printf("\nExample:\n");
    printf("  multi-gnss-sdr-sim -e brdc3540.14n -l 30.286502,120.032669,100 -c gps -d 60 -o test.bin\n");
}

// Main function
int main(int argc, char *argv[])
{
    gnss_config_t config;
    
    // Parse command line arguments
    int result = parse_command_line(argc, argv, &config);
    
    if (result < 0) {
        if (result == -1) {
            // Help or version was requested, exit normally
            return 0;
        } else {
            // Error in command line
            print_usage();
            return 1;
        }
    }
    
    // Validate configuration
    if (config.num_constellations == 0) {
        fprintf(stderr, "Error: No GNSS constellations selected\n");
        return 1;
    }
    
    // Print configuration
    printf("Multi-GNSS SDR Simulator Configuration:\n");
    printf("  Constellations: ");
    for (int i = 0; i < MAX_CONSTELLATIONS; i++) {
        if (config.constellations[i]) {
            switch (i) {
                case CONSTELLATION_GPS: printf("GPS "); break;
                case CONSTELLATION_GLONASS: printf("GLONASS "); break;
                case CONSTELLATION_GALILEO: printf("Galileo "); break;
                case CONSTELLATION_BEIDOU: printf("BeiDou "); break;
            }
        }
    }
    printf("\n");
    printf("  Location: %.6f°, %.6f°, %.1f m\n", 
           config.user_pos.llh[0], config.user_pos.llh[1], config.user_pos.llh[2]);
    printf("  Duration: %.1f seconds\n", config.duration);
    printf("  Sample Rate: %.0f Hz\n", config.sample_rate);
    printf("  Output: %s\n", config.output_file);
    
    if (config.verbose) {
        printf("\nInitializing simulator...\n");
    }
    
    // Initialize simulator
    if (multi_gnss_sim_init(&config) != 0) {
        fprintf(stderr, "Error: Failed to initialize simulator\n");
        return 1;
    }
    
    // Run simulation
    printf("\nStarting simulation...\n");
    result = multi_gnss_sim_run(&config);
    
    if (result != 0) {
        fprintf(stderr, "Error: Simulation failed\n");
        multi_gnss_sim_cleanup(&config);
        return 1;
    }
    
    // Cleanup
    multi_gnss_sim_cleanup(&config);
    
    printf("\nSimulation completed successfully!\n");
    printf("Output saved to: %s\n", config.output_file);
    
    return 0;
}