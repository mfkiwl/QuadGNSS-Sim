#ifndef RINEX_PARSER_H
#define RINEX_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include "quad_gnss_interface.h"

namespace QuadGNSS {

class RINEXParser {
public:
    static std::map<int, EphemerisData> parse_gps_rinex2(const std::string& filename);
    static std::map<int, EphemerisData> parse_rinex3(const std::string& filename, ConstellationType constellation);
    
public:
    static std::vector<std::string> split_line(const std::string& line, int chunk_size = 80);
    static double parse_dbl(const std::string& str, int pos, int len);
    static int parse_int(const std::string& str, int pos, int len);
    static std::string trim(const std::string& str);
    static bool is_gps_satellite(const std::string& sat_id);
    static bool is_galileo_satellite(const std::string& sat_id);
    static bool is_beidou_satellite(const std::string& sat_id);
};

} // namespace QuadGNSS

#endif // RINEX_PARSER_H