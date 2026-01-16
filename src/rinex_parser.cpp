#include "rinex_parser.h"
#include <iostream>
#include <iomanip>

namespace QuadGNSS {

std::vector<std::string> RINEXParser::split_line(const std::string& line, int chunk_size) {
    std::vector<std::string> chunks;
    for (size_t i = 0; i < line.length(); i += chunk_size) {
        chunks.push_back(line.substr(i, chunk_size));
    }
    return chunks;
}

double RINEXParser::parse_dbl(const std::string& str, int pos, int len) {
    std::string substr = str.substr(pos, len);
    // Replace 'D' with 'E' for scientific notation
    for (char& c : substr) {
        if (c == 'D' || c == 'd') c = 'E';
    }
    try {
        return std::stod(trim(substr));
    } catch (...) {
        return 0.0;
    }
}

int RINEXParser::parse_int(const std::string& str, int pos, int len) {
    try {
        return std::stoi(trim(str.substr(pos, len)));
    } catch (...) {
        return 0;
    }
}

std::string RINEXParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

bool RINEXParser::is_gps_satellite(const std::string& sat_id) {
    // GPS satellites in RINEX 2.11 start with PRN number (e.g., " 1", " 2", etc.)
    std::string trimmed = trim(sat_id);
    if (trimmed.length() == 1 && trimmed[0] >= '1' && trimmed[0] <= '9') {
        return true;
    }
    // Or format with 'G' prefix for RINEX 3.0
    return sat_id.length() >= 2 && sat_id[0] == 'G' && sat_id[1] >= '1' && sat_id[1] <= '9';
}

bool RINEXParser::is_galileo_satellite(const std::string& sat_id) {
    return sat_id.length() >= 2 && sat_id[0] == 'E' && sat_id[1] >= '1' && sat_id[1] <= '9';
}

bool RINEXParser::is_beidou_satellite(const std::string& sat_id) {
    return sat_id.length() >= 2 && sat_id[0] == 'C' && sat_id[1] >= '1' && sat_id[1] <= '9';
}

std::map<int, EphemerisData> RINEXParser::parse_gps_rinex2(const std::string& filename) {
    std::map<int, EphemerisData> ephemeris_data;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw QuadGNSSException("Cannot open RINEX file: " + filename);
    }
    
    std::string line;
    bool in_ephemeris_section = false;
    EphemerisData current_eph;
    
    while (std::getline(file, line)) {
        // Skip header lines
        if (line.find("END OF HEADER") != std::string::npos) {
            in_ephemeris_section = true;
            continue;
        }
        
        if (!in_ephemeris_section) continue;
        
        // Look for GPS ephemeris lines (start with PRN number)
        std::string sat_id = trim(line.substr(0, 3));  // Allow for 3 chars (e.g., "  1")
        if (line.length() > 22 && is_gps_satellite(sat_id)) {
            int prn = parse_int(line, 3, 2);  // PRN starts at position 3
            
            // Ephemeris record - parse orbital parameters
            if (line.length() > 68) {
                current_eph.prn = prn;
                current_eph.constellation = ConstellationType::GPS;
                current_eph.iodc = parse_dbl(line, 22, 19);
                current_eph.toc = parse_dbl(line, 41, 19);
                current_eph.clock_drift_rate = parse_dbl(line, 60, 19);
                
                // Read continuation lines
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.clock_drift = parse_dbl(line, 0, 19);
                    current_eph.clock_bias = parse_dbl(line, 19, 19);
                    current_eph.iode = parse_dbl(line, 38, 19);
                    current_eph.crs = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.delta_n = parse_dbl(line, 0, 19);
                    current_eph.m0 = parse_dbl(line, 19, 19);
                    current_eph.cuc = parse_dbl(line, 38, 19);
                    current_eph.e = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.cus = parse_dbl(line, 0, 19);
                    current_eph.sqrt_a = parse_dbl(line, 19, 19);
                    current_eph.toe = parse_dbl(line, 38, 19);
                    current_eph.cic = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.omega0 = parse_dbl(line, 0, 19);
                    current_eph.cis = parse_dbl(line, 19, 19);
                    current_eph.i0 = parse_dbl(line, 38, 19);
                    current_eph.crc = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.omega = parse_dbl(line, 0, 19);
                    current_eph.omega_dot = parse_dbl(line, 19, 19);
                    current_eph.idot = parse_dbl(line, 38, 19);
                    // Store completed ephemeris
                    current_eph.is_valid = true;
                    ephemeris_data[prn] = current_eph;
                }
            }
        }
    }
    
    file.close();
    return ephemeris_data;
}

std::map<int, EphemerisData> RINEXParser::parse_rinex3(const std::string& filename, ConstellationType constellation) {
    std::map<int, EphemerisData> ephemeris_data;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw QuadGNSSException("Cannot open RINEX 3 file: " + filename);
    }
    
    std::string line;
    bool in_ephemeris_section = false;
    EphemerisData current_eph;
    
    while (std::getline(file, line)) {
        // Skip header lines
        if (line.find("END OF HEADER") != std::string::npos) {
            in_ephemeris_section = true;
            continue;
        }
        
        if (!in_ephemeris_section) continue;
        
        // Check for constellation-specific satellites
        std::string sat_id = line.substr(0, 2);
        bool is_target_satellite = false;
        
        switch (constellation) {
            case ConstellationType::GALILEO:
                is_target_satellite = is_galileo_satellite(sat_id);
                break;
            case ConstellationType::BEIDOU:
                is_target_satellite = is_beidou_satellite(sat_id);
                break;
            case ConstellationType::GLONASS:
                is_target_satellite = (sat_id[0] == 'R');  // GLONASS satellites start with 'R'
                break;
            default:
                is_target_satellite = false;
                break;
        }
        
        if (is_target_satellite && line.length() > 22) {
            int prn = parse_int(line, 1, 2);  // PRN is after constellation letter
            
            // Ephemeris record - parse orbital parameters
            if (line.length() > 68) {
                current_eph.prn = prn;
                current_eph.constellation = constellation;
                
                // RINEX 3 format is similar to RINEX 2 but with different positioning
                current_eph.week_number = parse_dbl(line, 22, 19);
                current_eph.toc = parse_dbl(line, 41, 19);
                current_eph.clock_drift_rate = parse_dbl(line, 60, 19);
                
                // Read continuation lines (similar to RINEX 2 but with possible variations)
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.clock_drift = parse_dbl(line, 0, 19);
                    current_eph.clock_bias = parse_dbl(line, 19, 19);
                    current_eph.iode = parse_dbl(line, 38, 19);
                    current_eph.crs = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.delta_n = parse_dbl(line, 0, 19);
                    current_eph.m0 = parse_dbl(line, 19, 19);
                    current_eph.cuc = parse_dbl(line, 38, 19);
                    current_eph.e = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.cus = parse_dbl(line, 0, 19);
                    current_eph.sqrt_a = parse_dbl(line, 19, 19);
                    current_eph.toe = parse_dbl(line, 38, 19);
                    current_eph.cic = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.omega0 = parse_dbl(line, 0, 19);
                    current_eph.cis = parse_dbl(line, 19, 19);
                    current_eph.i0 = parse_dbl(line, 38, 19);
                    current_eph.crc = parse_dbl(line, 57, 19);
                }
                
                if (std::getline(file, line) && line.length() > 68) {
                    current_eph.omega = parse_dbl(line, 0, 19);
                    current_eph.omega_dot = parse_dbl(line, 19, 19);
                    current_eph.idot = parse_dbl(line, 38, 19);
                    // Store completed ephemeris
                    current_eph.is_valid = true;
                    ephemeris_data[prn] = current_eph;
                }
            }
        }
    }
    
    file.close();
    return ephemeris_data;
}

} // namespace QuadGNSS