#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/multi_gnss_sim.h"

// Default configuration values
static const double DEFAULT_LAT = 30.286502;
static const double DEFAULT_LON = 120.032669;
static const double DEFAULT_HEIGHT = 100.0;
static const int DEFAULT_DURATION = 300;
static const double DEFAULT_SAMPLE_RATE = 2600000.0;  // 2.6 MHz
static const int DEFAULT_IQ_BITS = 16;
static const double DEFAULT_TX_GAIN = 0.0;
static const double DEFAULT_OSCILLATOR_ERROR = 0.0;

static void print_usage_local(const char *program_name)
{
    printf("Usage: %s [options]\n\n", program_name);
    
    printf("Navigation Data:\n");
    printf("  -e <filename>     RINEX navigation file (required)\n");
    printf("  -f <url>          Download RINEX data from FTP server\n");
    printf("  -3                Use RINEX v3 format\n\n");
    
    printf("Constellation Selection:\n");
    printf("  -c <list>         GNSS constellations (gps,glonass,galileo,beidou,all)\n");
    printf("  --gps-only        GPS constellation only\n");
    printf("  --glonass-only    GLONASS constellation only\n\n");
    
    printf("Position & Motion:\n");
    printf("  -l <lat,lon,hgt>  Static location (degrees, meters)\n");
    printf("  -u <filename>     ECEF user motion file\n");
    printf("  -x <filename>     LLH user motion file\n");
    printf("  -g <filename>     NMEA GGA stream\n");
    printf("  -t <dist,bear,h>  Target relative position\n\n");
    
    printf("Timing:\n");
    printf("  -s <date,time>    Start time YYYY/MM/DD,hh:mm:ss\n");
    printf("  -d <seconds>      Duration\n");
    printf("  -p <ppb>          Oscillator error\n\n");
    
    printf("SDR Output:\n");
    printf("  -r <type>         SDR device (hackrf,pluto,bladerf,usrp,limesdr,iqfile)\n");
    printf("  -o <filename>     Output IQ file\n");
    printf("  -b <bits>         IQ resolution (1/8/16)\n");
    printf("  -g <gain>         TX gain\n\n");
    
    printf("Simulation:\n");
    printf("  -i                Disable ionospheric delay\n");
    printf("  -v                Verbose output\n");
    printf("  --interactive     Interactive mode\n");
    printf("  --help            Show this help message\n");
    printf("  --version         Show version information\n");
}

static void print_version(void)
{
    printf("Multi-GNSS SDR Simulator v1.0.0\n");
    printf("Copyright (c) 2024 Multi-GNSS Project\n");
    printf("License: MIT\n");
    printf("Support for GPS, GLONASS, Galileo, and BeiDou constellations\n");
}

static int parse_constellation_list(const char *list, bool *constellations)
{
    char *list_copy = strdup(list);
    char *token = strtok(list_copy, ",");
    
    while (token != NULL) {
        if (strcmp(token, "gps") == 0) {
            constellations[CONSTELLATION_GPS] = true;
        } else if (strcmp(token, "glonass") == 0) {
            constellations[CONSTELLATION_GLONASS] = true;
        } else if (strcmp(token, "galileo") == 0) {
            constellations[CONSTELLATION_GALILEO] = true;
        } else if (strcmp(token, "beidou") == 0) {
            constellations[CONSTELLATION_BEIDOU] = true;
        } else if (strcmp(token, "all") == 0) {
            for (int i = 0; i < MAX_CONSTELLATIONS; i++) {
                constellations[i] = true;
            }
        } else {
            free(list_copy);
            return -1;  // Invalid constellation name
        }
        token = strtok(NULL, ",");
    }
    
    free(list_copy);
    return 0;
}

static int parse_llh_string(const char *llh_str, double *llh)
{
    double lat, lon, height;
    
    if (sscanf(llh_str, "%lf,%lf,%lf", &lat, &lon, &height) != 3) {
        return -1;
    }
    
    // Validate ranges
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        return -1;
    }
    
    llh[0] = lat;
    llh[1] = lon;
    llh[2] = height;
    return 0;
}

static int parse_target_string(const char *target_str, double *target)
{
    double distance, bearing, height;
    
    if (sscanf(target_str, "%lf,%lf,%lf", &distance, &bearing, &height) != 3) {
        return -1;
    }
    
    // Validate ranges
    if (distance < 0.0 || bearing < 0.0 || bearing >= 360.0) {
        return -1;
    }
    
    target[0] = distance;
    target[1] = bearing * PI / 180.0;  // Convert to radians
    target[2] = height;
    return 0;
}

static sdr_device_t parse_sdr_type(const char *type_str)
{
    if (strcmp(type_str, "hackrf") == 0) return SDR_HACKRF;
    if (strcmp(type_str, "pluto") == 0) return SDR_PLUTO;
    if (strcmp(type_str, "bladerf") == 0) return SDR_BLADERF;
    if (strcmp(type_str, "usrp") == 0) return SDR_USRP;
    if (strcmp(type_str, "limesdr") == 0) return SDR_LIMESDR;
    if (strcmp(type_str, "iqfile") == 0) return SDR_IQFILE;
    
    return SDR_NONE;
}

void load_default_config(gnss_config_t *config)
{
    memset(config, 0, sizeof(gnss_config_t));
    
    // Navigation data
    config->rinex_version[0] = '\0';
    config->use_ftp = false;
    config->ftp_url[0] = '\0';
    
    // Constellation selection - enable GPS by default
    config->constellations[CONSTELLATION_GPS] = true;
    for (int i = 1; i < MAX_CONSTELLATIONS; i++) {
        config->constellations[i] = false;
    }
    config->num_constellations = 1;
    
    // Position
    config->user_pos.llh[0] = DEFAULT_LAT;
    config->user_pos.llh[1] = DEFAULT_LON;
    config->user_pos.llh[2] = DEFAULT_HEIGHT;
    llh_to_ecef(config->user_pos.llh, config->user_pos.xyz);
    config->static_mode = true;
    config->dynamic_mode = false;
    config->motion_file[0] = '\0';
    
    // Timing
    config->start_time = 0.0;  // Current time
    config->duration = DEFAULT_DURATION;
    config->oscillator_error_ppb = DEFAULT_OSCILLATOR_ERROR;
    
    // SDR output
    config->sdr_device = SDR_IQFILE;
    strcpy(config->output_file, "multi_gnss_output.bin");
    config->iq_bits = DEFAULT_IQ_BITS;
    config->tx_gain = DEFAULT_TX_GAIN;
    config->sample_rate = DEFAULT_SAMPLE_RATE;
    
    // Frequencies
    config->freq_gps = 1575420000.0;      // GPS L1
    config->freq_glonass = 1602000000.0;  // GLONASS L1 base
    config->freq_galileo = 1575420000.0;  // Galileo E1
    config->freq_beidou = 1561098000.0;   // BeiDou B1I
    
    // Simulation parameters
    config->disable_ionospheric = false;
    config->disable_tropospheric = false;
    config->verbose = false;
    config->interactive_mode = false;
    config->coherent_mode = true;
}

int parse_command_line(int argc, char *argv[], gnss_config_t *config)
{
    load_default_config(config);
    
    // Define command line options
    static struct option long_options[] = {
        {"help",            no_argument,       0, 'h'},
        {"version",         no_argument,       0, 'V'},
        {"verbose",         no_argument,       0, 'v'},
        {"interactive",     no_argument,       0, 1001},
        {"gps-only",        no_argument,       0, 1002},
        {"glonass-only",    no_argument,       0, 1003},
        
        {"rinex",           required_argument, 0, 'e'},
        {"ftp",             required_argument, 0, 'f'},
        {"rinex3",          no_argument,       0, '3'},
        
        {"constellations",  required_argument, 0, 'c'},
        
        {"llh",             required_argument, 0, 'l'},
        {"motion",          required_argument, 0, 'u'},
        {"llh-motion",      required_argument, 0, 'x'},
        {"nmea",            required_argument, 0, 'g'},
        {"target",          required_argument, 0, 't'},
        
        {"start",           required_argument, 0, 's'},
        {"duration",        required_argument, 0, 'd'},
        {"ppb",             required_argument, 0, 'p'},
        
        {"radio",           required_argument, 0, 'r'},
        {"output",          required_argument, 0, 'o'},
        {"bits",            required_argument, 0, 'b'},
        {"gain",            required_argument, 0, 1004},
        {"rate",            required_argument, 0, 1005},
        
        {"disable-iono",    no_argument,       0, 'i'},
        
        {0, 0, 0, 0}
    };
    
    int c, option_index = 0;
    
    while ((c = getopt_long(argc, argv, "hVv3e:f:c:l:u:x:g:t:s:d:p:r:o:b:gi", 
                          long_options, &option_index)) != -1) {
        
        switch (c) {
            case 'h':
                print_usage();
                return -1;
                
            case 'V':
                print_version();
                return -1;
                
            case 'v':
                config->verbose = true;
                break;
                
            case 1001:
                config->interactive_mode = true;
                break;
                
            case 1002:
                for (int i = 0; i < MAX_CONSTELLATIONS; i++) {
                    config->constellations[i] = false;
                }
                config->constellations[CONSTELLATION_GPS] = true;
                config->num_constellations = 1;
                break;
                
            case 1003:
                for (int i = 0; i < MAX_CONSTELLATIONS; i++) {
                    config->constellations[i] = false;
                }
                config->constellations[CONSTELLATION_GLONASS] = true;
                config->num_constellations = 1;
                break;
                
            case 'e':
                strncpy(config->nav_file, optarg, MAX_FILENAME - 1);
                break;
                
            case 'f':
                config->use_ftp = true;
                strncpy(config->ftp_url, optarg, MAX_FILENAME - 1);
                break;
                
            case '3':
                strcpy(config->rinex_version, "3");
                break;
                
            case 'c':
                if (parse_constellation_list(optarg, config->constellations) < 0) {
                    fprintf(stderr, "Error: Invalid constellation list\n");
                    return -2;
                }
                // Count enabled constellations
                config->num_constellations = 0;
                for (int i = 0; i < MAX_CONSTELLATIONS; i++) {
                    if (config->constellations[i]) {
                        config->num_constellations++;
                    }
                }
                break;
                
            case 'l':
                if (parse_llh_string(optarg, config->user_pos.llh) < 0) {
                    fprintf(stderr, "Error: Invalid LLH coordinates\n");
                    return -2;
                }
                llh_to_ecef(config->user_pos.llh, config->user_pos.xyz);
                config->static_mode = true;
                config->dynamic_mode = false;
                break;
                
            case 'u':
                strncpy(config->motion_file, optarg, MAX_FILENAME - 1);
                config->static_mode = false;
                config->dynamic_mode = true;
                break;
                
            case 'x':
                strncpy(config->motion_file, optarg, MAX_FILENAME - 1);
                config->static_mode = false;
                config->dynamic_mode = true;
                break;
                
            case 'g':
                strncpy(config->motion_file, optarg, MAX_FILENAME - 1);
                config->static_mode = false;
                config->dynamic_mode = true;
                break;
                
            case 's':
                config->start_time = parse_time_string(optarg);
                if (config->start_time < 0) {
                    fprintf(stderr, "Error: Invalid start time format\n");
                    return -2;
                }
                break;
                
            case 'd':
                config->duration = atoi(optarg);
                if (config->duration <= 0) {
                    fprintf(stderr, "Error: Duration must be positive\n");
                    return -2;
                }
                break;
                
            case 'p':
                config->oscillator_error_ppb = atof(optarg);
                break;
                
            case 'r':
                config->sdr_device = parse_sdr_type(optarg);
                if (config->sdr_device == SDR_NONE) {
                    fprintf(stderr, "Error: Invalid SDR device type\n");
                    return -2;
                }
                break;
                
            case 'o':
                strncpy(config->output_file, optarg, MAX_FILENAME - 1);
                break;
                
            case 'b':
                config->iq_bits = atoi(optarg);
                if (config->iq_bits != 1 && config->iq_bits != 8 && config->iq_bits != 16) {
                    fprintf(stderr, "Error: IQ bits must be 1, 8, or 16\n");
                    return -2;
                }
                break;
                
            case 1004:
                config->tx_gain = atof(optarg);
                break;
                
            case 1005:
                config->sample_rate = atof(optarg);
                break;
                
            case 'i':
                config->disable_ionospheric = true;
                break;
                
            case '?':
                return -2;
                
            default:
                fprintf(stderr, "Error: Unknown option\n");
                return -2;
        }
    }
    
    // Check for required options
    if (strlen(config->nav_file) == 0 && !config->use_ftp) {
        fprintf(stderr, "Error: RINEX navigation file is required\n");
        return -2;
    }
    
    return 0;
}