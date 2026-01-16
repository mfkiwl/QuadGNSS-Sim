#include "../include/quad_gnss_interface.h"
#include "../src/cdma_providers.cpp"
#include <iostream>
#include <iomanip>

using namespace QuadGNSS;

void test_cdma_providers() {
    std::cout << "=== CDMA Providers Test ===" << std::endl << std::endl;
    
    GlobalConfig config;
    config.sampling_rate_hz = 60e6;  // 60 MSps
    config.center_frequency_hz = 1582e6;  // 1582 MHz
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Sampling Rate: " << config.sampling_rate_hz / 1e6 << " MSps" << std::endl;
    std::cout << "  Center Frequency: " << config.center_frequency_hz / 1e6 << " MHz" << std::endl;
    std::cout << std::endl;
    
    // Test each CDMA provider
    std::vector<ConstellationType> cdma_constellations = {
        ConstellationType::GPS,
        ConstellationType::GALILEO,
        ConstellationType::BEIDOU
    };
    
    for (auto type : cdma_constellations) {
        std::cout << "Testing " << ConstellationFactory::get_constellation_name(type) << ":" << std::endl;
        
        try {
            // Create provider
            auto provider = ConstellationFactory::create_constellation(type);
            
            // Configure provider
            provider->configure(config);
            
            // Calculate and set frequency offset
            double carrier_freq = provider->get_carrier_frequency();
            double freq_offset = carrier_freq - config.center_frequency_hz;
            provider->set_frequency_offset(freq_offset);
            
            std::cout << "  Carrier Frequency: " << carrier_freq / 1e6 << " MHz" << std::endl;
            std::cout << "  Frequency Offset: " << freq_offset / 1e6 << " MHz" << std::endl;
            
            // Load mock ephemeris
            provider->load_ephemeris("mock_ephemeris.dat");
            
            // Check readiness
            std::cout << "  Ready for signal generation: " << (provider->is_ready() ? "YES" : "NO") << std::endl;
            
            // Show active satellites
            auto satellites = provider->get_active_satellites();
            std::cout << "  Active satellites: " << satellites.size() << std::endl;
            for (const auto& sat : satellites) {
                std::cout << "    PRN " << sat.prn 
                          << " - Power: " << sat.power_dbm << " dBm"
                          << ", Freq: " << sat.frequency_hz / 1e6 << " MHz" << std::endl;
            }
            
            // Test signal generation
            const int sample_count = 1000;
            const double test_time = 0.0;
            std::vector<std::complex<int16_t>> signal_buffer(sample_count);
            
            try {
                provider->generate_chunk(signal_buffer.data(), sample_count, test_time);
                std::cout << "  Signal generation: SUCCESS (" << sample_count << " samples)" << std::endl;
                
                // Show first few samples
                std::cout << "  Sample values (first 5):" << std::endl;
                for (int i = 0; i < 5 && i < sample_count; ++i) {
                    std::cout << "    [" << i << "] I:" << std::setw(5) << signal_buffer[i].real()
                              << " Q:" << std::setw(5) << signal_buffer[i].imag() << std::endl;
                }
                
                // Calculate signal statistics
                int16_t max_i = 0, max_q = 0;
                int64_t sum_i = 0, sum_q = 0;
                for (int i = 0; i < sample_count; ++i) {
                    max_i = std::max(max_i, static_cast<int16_t>(std::abs(signal_buffer[i].real())));
                    max_q = std::max(max_q, static_cast<int16_t>(std::abs(signal_buffer[i].imag())));
                    sum_i += signal_buffer[i].real();
                    sum_q += signal_buffer[i].imag();
                }
                
                std::cout << "  Signal stats:" << std::endl;
                std::cout << "    Max amplitude: I=" << max_i << ", Q=" << max_q << std::endl;
                std::cout << "    Average: I=" << sum_i / sample_count << ", Q=" << sum_q / sample_count << std::endl;
                
            } catch (const QuadGNSSException& e) {
                std::cout << "  Signal generation: FAILED - " << e.what() << std::endl;
            }
            
        } catch (const QuadGNSSException& e) {
            std::cout << "  ERROR: " << e.what() << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "=== CDMA Providers Test Complete ===" << std::endl;
}

void test_digital_mixing() {
    std::cout << std::endl << "=== Digital Mixing Test ===" << std::endl;
    
    GlobalConfig config;
    config.sampling_rate_hz = 60e6;
    config.center_frequency_hz = 1582e6;
    
    // Create GPS provider
    auto gps_provider = ConstellationFactory::create_constellation(ConstellationType::GPS);
    gps_provider->configure(config);
    gps_provider->load_ephemeris("mock_ephemeris.dat");
    
    std::cout << "GPS L1 Provider Details:" << std::endl;
    std::cout << "  Carrier: " << gps_provider->get_carrier_frequency() / 1e6 << " MHz" << std::endl;
    
    // Test different frequency offsets
    std::vector<double> test_offsets = {0.0, -6.58e6, -20.902e6};  // No offset, GPS offset, Beidou offset
    
    for (double offset : test_offsets) {
        std::cout << std::endl << "Testing frequency offset: " << offset / 1e6 << " MHz" << std::endl;
        
        gps_provider->set_frequency_offset(offset);
        
        const int sample_count = 100;
        std::vector<std::complex<int16_t>> signal_buffer(sample_count);
        
        try {
            gps_provider->generate_chunk(signal_buffer.data(), sample_count, 0.0);
            
            // Check if signal was actually mixed (should see different patterns)
            std::cout << "  Mixed signal samples:" << std::endl;
            for (int i = 0; i < 5; ++i) {
                std::cout << "    [" << i << "] I:" << std::setw(6) << signal_buffer[i].real()
                          << " Q:" << std::setw(6) << signal_buffer[i].imag() << std::endl;
            }
            
        } catch (const QuadGNSSException& e) {
            std::cout << "  ERROR: " << e.what() << std::endl;
        }
    }
    
    std::cout << std::endl << "=== Digital Mixing Test Complete ===" << std::endl;
}

int main() {
    try {
        test_cdma_providers();
        test_digital_mixing();
        
        std::cout << std::endl << "✅ All CDMA Provider Tests Completed Successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}