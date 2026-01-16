#ifndef GETOPT_H
#define GETOPT_H

#ifdef _WIN32

// getopt implementation for Windows
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;

#endif // _WIN32

#endif // GETOPT_H