#include <iostream>
#include <iomanip>
#include <complex>
#include <vector>
#include "quad_gnss_interface.h"

using namespace QuadGNSS;

// Simple test function to verify CDMA provider implementations
void test_cdma_implementations() {
    std::cout << "=== Testing CDMA Provider Implementations ===" << std::endl << std::endl;
    
    GlobalConfig config;
    config.sampling_rate_hz = 60e6;  // 60 MSps
    config.center_frequency_hz = 1581.5e6;  // 1581.5 MHz
    
    const int test_samples = 1000;  // Small test buffer
    std::vector<std::complex<int16_t>> test_buffer(test_samples);
    double test_time = 1000.0;  // 1000 seconds into simulation
    
    // Test GPS Provider
    std::cout << "Testing GPS L1 Provider..." << std::endl;
    try {
        auto gps_provider = ConstellationFactory::create_constellation(ConstellationType::GPS);
        gps_provider->configure(config);
        gps_provider->load_ephemeris("gps_test.dat");  // Dummy file
        
        if (gps_provider->is_ready()) {
            std::cout << "  ✓ GPS Provider is ready" << std::endl;
            gps_provider->generate_chunk(test_buffer.data(), test_samples, test_time);
            
            // Check if buffer has non-zero values
            bool has_signal = false;
            for (int i = 0; i < test_samples; ++i) {
                if (test_buffer[i].real() != 0 || test_buffer[i].imag() != 0) {
                    has_signal = true;
                    break;
                }
            }
            
            if (has_signal) {
                std::cout << "  ✓ GPS generates non-zero signal" << std::endl;
            } else {
                std::cout << "  ✗ GPS generates zero signal" << std::endl;
            }
        } else {
            std::cout << "  ✗ GPS Provider not ready" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "  ✗ GPS error: " << e.what() << std::endl;
    }
    std::cout << std::endl;
    
    // Test BeiDou Provider
    std::cout << "Testing BeiDou B1I Provider..." << std::endl;
    try {
        auto beidou_provider = ConstellationFactory::create_constellation(ConstellationType::BEIDOU);
        beidou_provider->configure(config);
        beidou_provider->load_ephemeris("beidou_test.dat");  // Dummy file
        
        if (beidou_provider->is_ready()) {
            std::cout << "  ✓ BeiDou Provider is ready" << std::endl;
            
            // Clear buffer
            std::fill(test_buffer.begin(), test_buffer.end(), std::complex<int16_t>(0, 0));
            
            beidou_provider->generate_chunk(test_buffer.data(), test_samples, test_time);
            
            // Check if buffer has non-zero values
            bool has_signal = false;
            for (int i = 0; i < test_samples; ++i) {
                if (test_buffer[i].real() != 0 || test_buffer[i].imag() != 0) {
                    has_signal = true;
                    break;
                }
            }
            
            if (has_signal) {
                std::cout << "  ✓ BeiDou generates non-zero signal" << std::endl;
            } else {
                std::cout << "  ✗ BeiDou generates zero signal" << std::endl;
            }
        } else {
            std::cout << "  ✗ BeiDou Provider not ready" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "  ✗ BeiDou error: " << e.what() << std::endl;
    }
    std::cout << std::endl;
    
    // Test Galileo Provider
    std::cout << "Testing Galileo E1 Provider..." << std::endl;
    try {
        auto galileo_provider = ConstellationFactory::create_constellation(ConstellationType::GALILEO);
        galileo_provider->configure(config);
        galileo_provider->load_ephemeris("galileo_test.dat");  // Dummy file
        
        if (galileo_provider->is_ready()) {
            std::cout << "  ✓ Galileo Provider is ready" << std::endl;
            
            // Clear buffer
            std::fill(test_buffer.begin(), test_buffer.end(), std::complex<int16_t>(0, 0));
            
            galileo_provider->generate_chunk(test_buffer.data(), test_samples, test_time);
            
            // Check if buffer has non-zero values
            bool has_signal = false;
            for (int i = 0; i < test_samples; ++i) {
                if (test_buffer[i].real() != 0 || test_buffer[i].imag() != 0) {
                    has_signal = true;
                    break;
                }
            }
            
            if (has_signal) {
                std::cout << "  ✓ Galileo generates non-zero signal" << std::endl;
            } else {
                std::cout << "  ✗ Galileo generates zero signal" << std::endl;
            }
        } else {
            std::cout << "  ✗ Galileo Provider not ready" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "  ✗ Galileo error: " << e.what() << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== CDMA Implementation Test Complete ===" << std::endl;
}

int main() {
    try {
        test_cdma_implementations();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}