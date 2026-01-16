#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    printf("Multi-GNSS SDR Simulator - Basic Test\n");
    printf("This is a test program to verify the build environment.\n");
    
    printf("\nArchitecture:");
    #if defined(_WIN32)
    printf(" Windows");
    #elif defined(__linux__)
    printf(" Linux");
    #elif defined(__APPLE__)
    printf(" macOS");
    #else
    printf(" Unknown");
    #endif
    
    printf("\nCompiler: ");
    #if defined(__GNUC__)
    printf("GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #elif defined(_MSC_VER)
    printf("MSVC %d", _MSC_VER);
    #else
    printf("Unknown");
    #endif
    
    printf("\nBuild successful!\n");
    return 0;
}