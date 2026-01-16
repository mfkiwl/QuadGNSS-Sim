#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/multi_gnss_sim.h"

// getopt implementation for cross-platform compatibility
char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = '?';

static char nextchar;
static const char *optstring = NULL;

static char *next_option = NULL;

int getopt(int argc, char * const argv[], const char *optstring)
{
    if (optind >= argc) {
        return -1;
    }
    
    char *option = argv[optind];
    
    // Handle non-option arguments
    if (option[0] != '-' || option[1] == '\0') {
        return -1;
    }
    
    // Single character option
    char opt = option[1];
    const char *p = strchr(optstring, opt);
    
    if (p == NULL) {
        optopt = opt;
        if (opterr) {
            fprintf(stderr, "Unknown option: -%c\n", opt);
        }
        return '?';
    }
    
    // Check if option requires argument
    if (p[1] == ':') {
        if (option[2] != '\0') {
            // Argument attached to option
            optarg = option + 2;
        } else if (optind + 1 < argc) {
            // Argument is next token
            optarg = argv[++optind];
        } else {
            // Missing argument
            if (opterr) {
                fprintf(stderr, "Option -%c requires an argument\n", opt);
            }
            return '?';
        }
    } else {
        optarg = NULL;
    }
    
    optind++;
    return opt;
}

int getopt_long(int argc, char * const argv[], const char *optstring, 
                const struct option *longopts, int *longindex)
{
    if (optind >= argc) {
        return -1;
    }
    
    char *option = argv[optind];
    
    // Check for long option (--option)
    if (strncmp(option, "--", 2) == 0) {
        char *option_name = option + 2;
        char *arg_part = strchr(option_name, '=');
        
        if (arg_part) {
            *arg_part = '\0';
            arg_part++;
        }
        
        // Find matching long option
        for (int i = 0; longopts[i].name != NULL; i++) {
            if (strcmp(option_name, longopts[i].name) == 0) {
                if (longindex) {
                    *longindex = i;
                }
                
                // Handle argument
                if (longopts[i].has_arg == required_argument) {
                    if (arg_part) {
                        optarg = arg_part;
                    } else if (optind + 1 < argc) {
                        optarg = argv[++optind];
                    } else {
                        if (opterr) {
                            fprintf(stderr, "Option --%s requires an argument\n", longopts[i].name);
                        }
                        return '?';
                    }
                } else if (longopts[i].has_arg == optional_argument) {
                    optarg = arg_part;
                } else {
                    optarg = NULL;
                }
                
                optind++;
                return longopts[i].val;
            }
        }
        
        optopt = 0;
        if (opterr) {
            fprintf(stderr, "Unknown option: --%s\n", option_name);
        }
        return '?';
    }
    
    // Fall back to short option parsing
    return getopt(argc, argv, optstring);
}