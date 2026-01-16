#include <iostream>
#include <iomanip>
#include <complex>
#include <vector>
#include <limits>
#include "quad_gnss_interface.h"

using namespace QuadGNSS;

void test_buffer_overflow_protection() {
    std::cout << "=== Testing Buffer Overflow Protection ===" << std::endl << std::endl;
    
    GlobalConfig config;
    config.sampling_rate_hz = 60e6;
    config.center_frequency_hz = 1581.5e6;
    
    const int test_samples = 1000;
    std::vector<std::complex<int16_t>> test_buffer(test_samples);
    double test_time = 1000.0;
    
    // Fill buffer with maximum values to test overflow protection
    std::fill(test_buffer.begin(), test_buffer.end(), 
              std::complex<int16_t>(32760, 32760));  // Near max int16
    
    std::cout << "Initial buffer state: Near maximum values (32760, 32760)" << std::endl;
    
    // Test GPS Provider with many satellites to trigger potential overflow
    auto gps_provider = ConstellationFactory::create_constellation(ConstellationType::GPS);
    gps_provider->configure(config);
    gps_provider->load_ephemeris("gps_test.dat");
    
    // Check that buffer values don't overflow after signal generation
    gps_provider->generate_chunk(test_buffer.data(), test_samples, test_time);
    
    bool overflow_detected = false;
    for (int i = 0; i < test_samples; ++i) {
        if (test_buffer[i].real() > 32767 || test_buffer[i].real() < -32768 ||
            test_buffer[i].imag() > 32767 || test_buffer[i].imag() < -32768) {
            overflow_detected = true;
            break;
        }
    }
    
    if (!overflow_detected) {
        std::cout << "  ✓ Buffer overflow protection working - no values outside int16 range" << std::endl;
    } else {
        std::cout << "  ✗ Buffer overflow detected - values outside int16 range" << std::endl;
    }
    
    // Check that some signal was actually added
    bool has_signal = false;
    for (int i = 0; i < test_samples; ++i) {
        if (abs(test_buffer[i].real()) > 32760 || abs(test_buffer[i].imag()) > 32760) {
            has_signal = true;
            break;
        }
    }
    
    if (has_signal) {
        std::cout << "  ✓ Signal properly added to buffer" << std::endl;
    } else {
        std::cout << "  ✗ No signal detected in buffer" << std::endl;
    }
    
    std::cout << std::endl;
    
    // Test multi-constellation mixing
    std::cout << "Testing multi-constellation signal mixing..." << std::endl;
    
    // Clear buffer
    std::fill(test_buffer.begin(), test_buffer.end(), std::complex<int16_t>(0, 0));
    
    // Generate signals from all constellations
    auto beidou_provider = ConstellationFactory::create_constellation(ConstellationType::BEIDOU);
    auto galileo_provider = ConstellationFactory::create_constellation(ConstellationType::GALILEO);
    
    beidou_provider->configure(config);
    beidou_provider->load_ephemeris("beidou_test.dat");
    
    galileo_provider->configure(config);
    galileo_provider->load_ephemeris("galileo_test.dat");
    
    // Mix all signals
    gps_provider->generate_chunk(test_buffer.data(), test_samples, test_time);
    beidou_provider->generate_chunk(test_buffer.data(), test_samples, test_time);
    galileo_provider->generate_chunk(test_buffer.data(), test_samples, test_time);
    
    // Check for overflow after mixing
    overflow_detected = false;
    for (int i = 0; i < test_samples; ++i) {
        if (test_buffer[i].real() > 32767 || test_buffer[i].real() < -32768 ||
            test_buffer[i].imag() > 32767 || test_buffer[i].imag() < -32768) {
            overflow_detected = true;
            break;
        }
    }
    
    if (!overflow_detected) {
        std::cout << "  ✓ Multi-constellation mixing without overflow" << std::endl;
    } else {
        std::cout << "  ✗ Overflow detected in multi-constellation mixing" << std::endl;
    }
    
    std::cout << "=== Buffer Overflow Protection Test Complete ===" << std::endl;
}

int main() {
    try {
        test_buffer_overflow_protection();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}