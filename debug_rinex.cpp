#include <iostream>
#include <fstream>
#include <string>
#include "quad_gnss_interface.h"
#include "rinex_parser.h"

using namespace QuadGNSS;

int main() {
    std::cout << "=== Debug RINEX Parsing ===" << std::endl;
    
    try {
        std::ifstream file("gps_ephemeris.dat");
        std::string line;
        int line_count = 0;
        
        while (std::getline(file, line) && line_count < 20) {
            std::cout << "Line " << line_count << ": '" << line << "'" << std::endl;
            
            // Test satellite detection
            if (line.length() > 22) {
                std::string sat_id = line.substr(0, 2);
                std::cout << "  Sat ID: '" << sat_id << "' - is_gps: " << RINEXParser::is_gps_satellite(sat_id) << std::endl;
            }
            
            line_count++;
        }
        
        file.close();
        
        // Test parsing
        std::cout << "\n=== Testing Parse Function ===" << std::endl;
        auto ephemeris = RINEXParser::parse_gps_rinex2("gps_ephemeris.dat");
        std::cout << "Parsed " << ephemeris.size() << " records" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}