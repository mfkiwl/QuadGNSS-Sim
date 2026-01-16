#include "../include/quad_gnss_interface.h"
#include <iostream>

using namespace QuadGNSS;

int main() {
    try {
        // Test configuration
        GlobalConfig config;
        std::cout << "QuadGNSS Interface Test" << std::endl;
        std::cout << "Sampling Rate: " << config.sampling_rate_hz << " Hz" << std::endl;
        std::cout << "Center Frequency: " << config.center_frequency_hz << " Hz" << std::endl;
        std::cout << "Active Constellations: " << config.active_constellations.size() << std::endl;
        
        // Test factory
        auto gps_constellation = ConstellationFactory::create_constellation(ConstellationType::GPS);
        std::cout << "GPS Frequency: " << gps_constellation->get_carrier_frequency() << " Hz" << std::endl;
        std::cout << "GPS Name: " << ConstellationFactory::get_constellation_name(ConstellationType::GPS) << std::endl;
        
        // Test orchestrator
        SignalOrchestrator orchestrator(config);
        orchestrator.add_constellation(std::move(gps_constellation));
        
        std::cout << "Constellation count: " << orchestrator.get_constellation_count() << std::endl;
        
        std::cout << "Interface test completed successfully!" << std::endl;
        return 0;
        
    } catch (const QuadGNSSException& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
        return 1;
    }
}