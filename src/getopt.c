#ifdef _WIN32

#include <string.h>
#include <stdio.h>

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = '?';

static char nextchar;

int getopt(int argc, char * const argv[], const char *optstring)
{
    static char *next_option = NULL;
    
    if (next_option == NULL || *next_option == '\0') {
        if (optind >= argc) {
            return -1;
        }
        
        char *option = argv[optind];
        
        if (option[0] != '-' || option[1] == '\0') {
            return -1;
        }
        
        next_option = option + 1;
        optind++;
    }
    
    char c = *next_option++;
    const char *p = strchr(optstring, c);
    
    if (p == NULL) {
        optopt = c;
        if (opterr) {
            fprintf(stderr, "Unknown option: -%c\n", c);
        }
        return '?';
    }
    
    if (p[1] == ':') {
        if (*next_option != '\0') {
            optarg = next_option;
            next_option = NULL;
        } else if (optind < argc) {
            optarg = argv[optind++];
        } else {
            if (opterr) {
                fprintf(stderr, "Option -%c requires an argument\n", c);
            }
            return '?';
        }
    }
    
    return c;
}

#endif // _WIN32