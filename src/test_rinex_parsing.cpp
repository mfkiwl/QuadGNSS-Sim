#include <iostream>
#include <iomanip>
#include "quad_gnss_interface.h"
#include "rinex_parser.h"

using namespace QuadGNSS;

void test_rinex_parsing() {
    std::cout << "=== Testing RINEX Parsing ===" << std::endl << std::endl;
    
    try {
        // Test GPS RINEX 2.11 parsing
        std::cout << "Testing GPS RINEX 2.11 parser..." << std::endl;
        auto gps_ephemeris = RINEXParser::parse_gps_rinex2("gps_ephemeris.dat");
        
        std::cout << "  Parsed " << gps_ephemeris.size() << " GPS ephemeris records:" << std::endl;
        for (const auto& [prn, eph] : gps_ephemeris) {
            std::cout << "    PRN " << prn << ": "
                      << "SqrtA=" << std::fixed << std::setprecision(1) << eph.sqrt_a
                      << ", e=" << std::scientific << eph.e
                      << ", Toe=" << std::fixed << eph.toe
                      << ", Valid=" << (eph.is_valid ? "Yes" : "No") << std::endl;
        }
        std::cout << std::endl;
        
        // Test Galileo RINEX 3.0 parsing
        std::cout << "Testing Galileo RINEX 3.0 parser..." << std::endl;
        auto galileo_ephemeris = RINEXParser::parse_rinex3("galileo_ephemeris.dat", ConstellationType::GALILEO);
        
        std::cout << "  Parsed " << galileo_ephemeris.size() << " Galileo ephemeris records:" << std::endl;
        for (const auto& [prn, eph] : galileo_ephemeris) {
            std::cout << "    PRN " << prn << ": "
                      << "SqrtA=" << std::fixed << std::setprecision(1) << eph.sqrt_a
                      << ", e=" << std::scientific << eph.e
                      << ", Toe=" << std::fixed << eph.toe
                      << ", Valid=" << (eph.is_valid ? "Yes" : "No") << std::endl;
        }
        std::cout << std::endl;
        
        // Test BeiDou RINEX 3.0 parsing
        std::cout << "Testing BeiDou RINEX 3.0 parser..." << std::endl;
        auto beidou_ephemeris = RINEXParser::parse_rinex3("beidou_ephemeris.dat", ConstellationType::BEIDOU);
        
        std::cout << "  Parsed " << beidou_ephemeris.size() << " BeiDou ephemeris records:" << std::endl;
        for (const auto& [prn, eph] : beidou_ephemeris) {
            std::cout << "    PRN " << prn << ": "
                      << "SqrtA=" << std::fixed << std::setprecision(1) << eph.sqrt_a
                      << ", e=" << std::scientific << eph.e
                      << ", Toe=" << std::fixed << eph.toe
                      << ", Valid=" << (eph.is_valid ? "Yes" : "No") << std::endl;
        }
        std::cout << std::endl;
        
        // Test integration with CDMA providers
        std::cout << "Testing integration with CDMA providers..." << std::endl;
        GlobalConfig config;
        config.sampling_rate_hz = 60e6;
        config.center_frequency_hz = 1581.5e6;
        
        // Test GPS provider with real ephemeris loading
        auto gps_provider = ConstellationFactory::create_constellation(ConstellationType::GPS);
        gps_provider->configure(config);
        gps_provider->load_ephemeris("gps_ephemeris.dat");
        
        if (gps_provider->is_ready()) {
            std::cout << "  ✓ GPS provider ready with ephemeris data" << std::endl;
        } else {
            std::cout << "  ✗ GPS provider not ready" << std::endl;
        }
        
        // Test Galileo provider with real ephemeris loading
        auto galileo_provider = ConstellationFactory::create_constellation(ConstellationType::GALILEO);
        galileo_provider->configure(config);
        galileo_provider->load_ephemeris("galileo_ephemeris.dat");
        
        if (galileo_provider->is_ready()) {
            std::cout << "  ✓ Galileo provider ready with ephemeris data" << std::endl;
        } else {
            std::cout << "  ✗ Galileo provider not ready" << std::endl;
        }
        
        // Test BeiDou provider with real ephemeris loading
        auto beidou_provider = ConstellationFactory::create_constellation(ConstellationType::BEIDOU);
        beidou_provider->configure(config);
        beidou_provider->load_ephemeris("beidou_ephemeris.dat");
        
        if (beidou_provider->is_ready()) {
            std::cout << "  ✓ BeiDou provider ready with ephemeris data" << std::endl;
        } else {
            std::cout << "  ✗ BeiDou provider not ready" << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "=== RINEX Parsing Test Complete ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
    }
}

int main() {
    test_rinex_parsing();
    return 0;
}