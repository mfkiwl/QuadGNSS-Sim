#include "../include/quad_gnss_interface.h"
#include "../src/cdma_providers.cpp"
#include <iostream>
#include <iomanip>

using namespace QuadGNSS;

void demonstrate_cdma_integration() {
    std::cout << "=== QuadGNSS-Sim CDMA Integration Demo ===" << std::endl << std::endl;
    
    // 1. Setup global configuration
    GlobalConfig config;
    config.sampling_rate_hz = 60e6;      // 60 MSps
    config.center_frequency_hz = 1582e6;  // 1582 MHz (master LO)
    
    std::cout << "Master Configuration:" << std::endl;
    std::cout << "  Sampling Rate: " << config.sampling_rate_hz / 1e6 << " MSps" << std::endl;
    std::cout << "  Center Frequency (LO): " << config.center_frequency_hz / 1e6 << " MHz" << std::endl;
    std::cout << "  Signal Duration: " << config.simulation.duration_seconds << " seconds" << std::endl;
    std::cout << std::endl;
    
    // 2. Create individual CDMA providers
    std::vector<std::unique_ptr<ISatelliteConstellation>> providers;
    std::vector<ConstellationType> cdma_constellations = {
        ConstellationType::GPS,
        ConstellationType::GALILEO,
        ConstellationType::BEIDOU
    };
    
    std::cout << "Creating CDMA Providers:" << std::endl;
    
    for (auto type : cdma_constellations) {
        try {
            auto provider = ConstellationFactory::create_constellation(type);
            std::cout << "  ✓ Created " << ConstellationFactory::get_constellation_name(type) 
                      << " (" << provider->get_carrier_frequency() / 1e6 << " MHz)" << std::endl;
            providers.push_back(std::move(provider));
        } catch (const QuadGNSSException& e) {
            std::cout << "  ❌ Failed to create " << ConstellationFactory::get_constellation_name(type)
                      << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "  Total providers: " << providers.size() << std::endl;
    std::cout << std::endl;
    
    // 3. Configure and initialize providers
    std::cout << "Initializing Providers:" << std::endl;
    std::map<ConstellationType, std::string> ephemeris_files = {
        {ConstellationType::GPS,     "gps_brdc3540.23n"},
        {ConstellationType::GALILEO, "galileo_brdc3540.23l"},
        {ConstellationType::BEIDOU,  "beidou_brdc3540.23b"}
    };
    
    for (auto& provider : providers) {
        try {
            provider->configure(config);
            
            // Calculate and set frequency offset
            double carrier_freq = provider->get_carrier_frequency();
            double freq_offset = carrier_freq - config.center_frequency_hz;
            provider->set_frequency_offset(freq_offset);
            
            // Load mock ephemeris
            auto type = provider->get_constellation_type();
            auto it = ephemeris_files.find(type);
            if (it != ephemeris_files.end()) {
                provider->load_ephemeris(it->second);
            }
            
            std::cout << "  ✓ " << ConstellationFactory::get_constellation_name(type)
                      << " ready: " << (provider->is_ready() ? "YES" : "NO") << std::endl;
            
        } catch (const QuadGNSSException& e) {
            std::cout << "  ⚠ Warning: " << e.what() << std::endl;
        }
    }
    
    std::cout << std::endl;
    
    // 4. Show frequency multiplexing
    std::cout << "Frequency Multiplexing Configuration:" << std::endl;
    std::cout << "  Master LO: " << config.center_frequency_hz / 1e6 << " MHz" << std::endl;
    
    for (const auto& provider : providers) {
        double carrier_freq = provider->get_carrier_frequency();
        double offset = carrier_freq - config.center_frequency_hz;
        auto type = provider->get_constellation_type();
        
        std::cout << "  " << std::setw(8) << ConstellationFactory::get_constellation_name(type)
                  << ": " << std::setw(8) << (carrier_freq / 1e6) << " MHz"
                  << " → Offset: " << std::setw(7) << (offset / 1e6) << " MHz" << std::endl;
    }
    
    std::cout << std::endl;
    
    // 5. Generate individual signals
    std::cout << "Signal Generation Test:" << std::endl;
    const int sample_count = 10000;  // 10K samples
    const double current_time = 0.0;
    std::vector<std::vector<std::complex<int16_t>>> individual_signals;
    
    for (size_t i = 0; i < providers.size(); ++i) {
        try {
            std::vector<std::complex<int16_t>> signal(sample_count);
            providers[i]->generate_chunk(signal.data(), sample_count, current_time);
            individual_signals.push_back(std::move(signal));
            
            auto type = providers[i]->get_constellation_type();
            std::cout << "  ✓ Generated " << ConstellationFactory::get_constellation_name(type)
                      << " signal (" << sample_count << " samples)" << std::endl;
            
        } catch (const QuadGNSSException& e) {
            std::cout << "  ❌ Failed to generate " 
                      << ConstellationFactory::get_constellation_name(providers[i]->get_constellation_type())
                      << " signal: " << e.what() << std::endl;
        }
    }
    
    std::cout << std::endl;
    
    // 6. Mix all signals manually (demonstrating orchestrator functionality)
    std::cout << "Signal Mixing:" << std::endl;
    std::vector<std::complex<int32_t>> accumulator(sample_count);
    std::fill(accumulator.begin(), accumulator.end(), std::complex<int32_t>(0, 0));
    
    for (const auto& signal : individual_signals) {
        for (int i = 0; i < sample_count; ++i) {
            accumulator[i] += std::complex<int32_t>(signal[i].real(), signal[i].imag());
        }
    }
    
    // Convert to int16_t with overflow protection
    std::vector<std::complex<int16_t>> mixed_signal(sample_count);
    const int32_t max_val = std::numeric_limits<int16_t>::max();
    const int32_t min_val = std::numeric_limits<int16_t>::min();
    
    for (int i = 0; i < sample_count; ++i) {
        int32_t real = accumulator[i].real();
        int32_t imag = accumulator[i].imag();
        
        real = std::max(min_val, std::min(max_val, real));
        imag = std::max(min_val, std::min(max_val, imag));
        
        mixed_signal[i] = std::complex<int16_t>(real, imag);
    }
    
    std::cout << "  ✓ Mixed " << individual_signals.size() << " signals successfully" << std::endl;
    
    // Calculate signal statistics
    int16_t max_i = 0, max_q = 0;
    int64_t sum_i = 0, sum_q = 0;
    for (int i = 0; i < sample_count; ++i) {
        max_i = std::max(max_i, static_cast<int16_t>(std::abs(mixed_signal[i].real())));
        max_q = std::max(max_q, static_cast<int16_t>(std::abs(mixed_signal[i].imag())));
        sum_i += mixed_signal[i].real();
        sum_q += mixed_signal[i].imag();
    }
    
    std::cout << "  Mixed Signal Statistics:" << std::endl;
    std::cout << "    Max amplitude: I=" << max_i << ", Q=" << max_q << std::endl;
    std::cout << "    Average: I=" << (sum_i / sample_count) << ", Q=" << (sum_q / sample_count) << std::endl;
    std::cout << "    Sample rate: " << config.sampling_rate_hz / 1e6 << " MSps" << std::endl;
    std::cout << "    Duration: " << sample_count / config.sampling_rate_hz * 1000 << " ms" << std::endl;
    
    // Show first few samples demonstrating frequency multiplexing
    std::cout << "  Sample Values (first 8 - showing mixed constellation signals):" << std::endl;
    for (int i = 0; i < 8; ++i) {
        std::cout << "    [" << std::setw(2) << i << "] I:" << std::setw(6) << mixed_signal[i].real()
                  << " Q:" << std::setw(6) << mixed_signal[i].imag() << std::endl;
    }
    
    std::cout << std::endl;
    
    // 7. Show active satellites summary
    std::cout << "Active Satellites Summary:" << std::endl;
    std::map<ConstellationType, int> sat_counts;
    
    for (const auto& provider : providers) {
        if (provider->is_ready()) {
            auto sats = provider->get_active_satellites();
            sat_counts[provider->get_constellation_type()] = sats.size();
        }
    }
    
    for (const auto& [type, count] : sat_counts) {
        std::cout << "  " << std::setw(8) << ConstellationFactory::get_constellation_name(type)
                  << ": " << count << " satellites" << std::endl;
    }
    
    int total_sats = 0;
    for (const auto& [type, count] : sat_counts) {
        total_sats += count;
    }
    std::cout << "  Total: " << total_sats << " active satellites" << std::endl;
    
    std::cout << std::endl << "=== CDMA Integration Demo Complete ===" << std::endl;
    std::cout << std::endl << "📡 QuadGNSS-Sim is ready for multi-GNSS signal generation!" << std::endl;
}

int main() {
    try {
        demonstrate_cdma_integration();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
}