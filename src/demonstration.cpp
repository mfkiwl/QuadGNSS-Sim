#include "../include/quad_gnss_interface.h"
#include <iostream>
#include <iomanip>

using namespace QuadGNSS;

void demonstrate_interface_usage() {
    std::cout << "=== QuadGNSS-Sim Interface Demonstration ===" << std::endl << std::endl;
    
    // 1. Create and configure global configuration
    GlobalConfig config;
    std::cout << "Global Configuration:" << std::endl;
    std::cout << "  Sampling Rate: " << config.sampling_rate_hz / 1e6 << " MSps" << std::endl;
    std::cout << "  Center Frequency: " << config.center_frequency_hz / 1e6 << " MHz" << std::endl;
    std::cout << "  Active Constellations: " << config.active_constellations.size() << std::endl;
    std::cout << "  Duration: " << config.simulation.duration_seconds << " seconds" << std::endl;
    std::cout << std::endl;
    
    // 2. Create constellation factory instances
    std::cout << "Constellation Factory Test:" << std::endl;
    std::vector<ConstellationType> constellations = {
        ConstellationType::GPS,
        ConstellationType::GLONASS,
        ConstellationType::GALILEO,
        ConstellationType::BEIDOU
    };
    
    for (auto type : constellations) {
        try {
            auto constellation = ConstellationFactory::create_constellation(type);
            std::cout << "  " << std::setw(8) << ConstellationFactory::get_constellation_name(type)
                      << ": " << std::setw(10) << (constellation->get_carrier_frequency() / 1e6)
                      << " MHz" << std::endl;
        } catch (const QuadGNSSException& e) {
            std::cerr << "  Error creating " << ConstellationFactory::get_constellation_name(type)
                      << ": " << e.what() << std::endl;
        }
    }
    std::cout << std::endl;
    
    // 3. Create and configure SignalOrchestrator
    std::cout << "Signal Orchestrator Setup:" << std::endl;
    SignalOrchestrator orchestrator(config);
    
    // Add constellations to orchestrator
    for (auto type : constellations) {
        try {
            auto constellation = ConstellationFactory::create_constellation(type);
            orchestrator.add_constellation(std::move(constellation));
            std::cout << "  Added " << ConstellationFactory::get_constellation_name(type) 
                      << " constellation" << std::endl;
        } catch (const QuadGNSSException& e) {
            std::cerr << "  Error adding " << ConstellationFactory::get_constellation_name(type)
                      << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "  Total constellations: " << orchestrator.get_constellation_count() << std::endl;
    std::cout << std::endl;
    
    // 4. Initialize with mock ephemeris files
    std::cout << "Initialization Test:" << std::endl;
    std::map<ConstellationType, std::string> ephemeris_files = {
        {ConstellationType::GPS, "gps_ephemeris.dat"},
        {ConstellationType::GLONASS, "glonass_ephemeris.dat"},
        {ConstellationType::GALILEO, "galileo_ephemeris.dat"},
        {ConstellationType::BEIDOU, "beidou_ephemeris.dat"}
    };
    
    try {
        orchestrator.initialize(ephemeris_files);
        std::cout << "  Orchestrator ready: " << (orchestrator.is_ready() ? "YES" : "NO") << std::endl;
    } catch (const QuadGNSSException& e) {
        std::cout << "  Initialization failed (expected): " << e.what() << std::endl;
    }
    std::cout << std::endl;
    
    // 5. Display signal generation capabilities
    std::cout << "Signal Generation Capabilities:" << std::endl;
    auto all_satellites = orchestrator.get_all_satellites();
    std::cout << "  Total active satellites: " << all_satellites.size() << std::endl;
    
    for (const auto& sat : all_satellites) {
        std::cout << "    PRN " << std::setw(2) << sat.prn 
                  << " (" << std::setw(8) << ConstellationFactory::get_constellation_name(sat.constellation)
                  << ") - Power: " << std::setw(6) << sat.power_dbm << " dBm"
                  << ", Doppler: " << std::setw(7) << sat.doppler_hz << " Hz" << std::endl;
    }
    std::cout << std::endl;
    
    // 6. Signal generation test
    std::cout << "Signal Generation Test:" << std::endl;
    const int sample_count = 1000;
    const double current_time = 0.0;
    std::vector<std::complex<int16_t>> signal_buffer(sample_count);
    
    try {
        orchestrator.mix_all_signals(signal_buffer.data(), sample_count, current_time);
        std::cout << "  Generated " << sample_count << " IQ samples successfully" << std::endl;
        
        // Show a few sample values
        std::cout << "  Sample values (first 5):" << std::endl;
        for (int i = 0; i < 5 && i < sample_count; ++i) {
            std::cout << "    [" << i << "] I:" << std::setw(6) << signal_buffer[i].real()
                      << " Q:" << std::setw(6) << signal_buffer[i].imag() << std::endl;
        }
    } catch (const QuadGNSSException& e) {
        std::cout << "  Signal generation failed (expected for demo): " << e.what() << std::endl;
    }
    
    std::cout << std::endl << "=== Interface Demonstration Complete ===" << std::endl;
}

int main() {
    try {
        demonstrate_interface_usage();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}