#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <complex>
#include <vector>

// Simple definitions for demo
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Configuration for broad-spectrum GNSS spoofing
struct BroadSpectrumConfig {
    static constexpr double SAMPLE_RATE_HZ = 60.0e6;          // 60 MSps
    static constexpr double CENTER_FREQ_HZ = 1581.5e6;       // 1581.5 MHz
    static constexpr double CHUNK_DURATION_SEC = 0.01;          // 10ms chunks
    
    // Power weighting (relative multipliers)
    static constexpr double GPS_WEIGHT = 1.0;               // GPS L1 strongest
    static constexpr double GALILEO_WEIGHT = 1.0;              // Galileo E1
    static constexpr double BEIDOU_WEIGHT = 1.0;              // BeiDou B1
    static constexpr double GLONASS_WEIGHT = 0.8;             // GLONASS L1 slightly attenuated
    
    static constexpr int CHUNK_SIZE = static_cast<int>(SAMPLE_RATE_HZ * CHUNK_DURATION_SEC);
};

// Simple signal generation class
class GNSSSignalGenerator {
private:
    double sample_rate_;
    double current_time_;
    bool running_;
    
public:
    GNSSSignalGenerator() : sample_rate_(BroadSpectrumConfig::SAMPLE_RATE_HZ), 
                           current_time_(0.0), running_(false) {}
    
    void start() {
        running_ = true;
        std::cout << "=== QuadGNSS Broad-Spectrum Generator ===" << std::endl << std::endl;
        
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Sample Rate: " << sample_rate_ / 1e6 << " MSps" << std::endl;
        std::cout << "  Center Frequency: " << (BroadSpectrumConfig::CENTER_FREQ_HZ / 1e6) << " MHz" << std::endl;
        std::cout << "  Chunk Duration: " << (BroadSpectrumConfig::CHUNK_DURATION_SEC * 1000) << " ms" << std::endl;
        std::cout << "  Chunk Size: " << BroadSpectrumConfig::CHUNK_SIZE << " samples" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Signal Power Weights:" << std::endl;
        std::cout << "  GPS:     " << BroadSpectrumConfig::GPS_WEIGHT << "x (-158.5 dBW typical)" << std::endl;
        std::cout << "  Galileo:  " << BroadSpectrumConfig::GALILEO_WEIGHT << "x" << std::endl;
        std::cout << "  BeiDou:   " << BroadSpectrumConfig::BEIDOU_WEIGHT << "x" << std::endl;
        std::cout << "  GLONASS:  " << BroadSpectrumConfig::GLONASS_WEIGHT << "x (slightly attenuated)" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Constellation Frequency Plan:" << std::endl;
        std::cout << "  GPS L1:     1575.42 MHz → Δf: -6.08 MHz" << std::endl;
        std::cout << "  GLONASS L1:  1602 MHz   → Δf: +20.5 MHz" << std::endl;
        std::cout << "  Galileo E1:  1575.42 MHz → Δf: -6.08 MHz" << std::endl;
        std::cout << "  BeiDou B1:    1561.1 MHz → Δf: -20.4 MHz" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Starting Signal Generation:" << std::endl;
        std::cout << "  Output format: Interleaved Signed 16-bit IQ to stdout" << std::endl;
        std::cout << "  Press Ctrl+C to stop generation" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Signal Generation Started:" << std::endl;
        std::cout << "┌─────────────────────────────────────────────┐" << std::endl;
        std::cout << "│ Time(s) │ Satellites │ Signal Samples Generated │" << std::endl;
        std::cout << "├─────────────────────────────────────────────┤" << std::endl;
        
        // Infinite generation loop
        int chunk_count = 0;
        auto last_status_time = std::chrono::steady_clock::now();
        
        while (running_) {
            try {
                // Generate mixed signal
                std::vector<std::complex<int16_t>> signal_chunk = generate_chunk();
                
                // Output interleaved IQ data to stdout
                output_signal_to_stdout(signal_chunk);
                
                // Update time
                current_time_ += BroadSpectrumConfig::CHUNK_DURATION_SEC;
                chunk_count++;
                
                // Status update every 1 second
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_status_time).count();
                
                if (elapsed >= 1) {
                    int total_satellites = 19;  // Simulated active satellites
                    std::cout << "│ " << std::setw(7) << std::fixed << std::setprecision(3) << current_time_
                              << " │ " << std::setw(10) << total_satellites
                              << " │ " << std::setw(21) << (chunk_count * BroadSpectrumConfig::CHUNK_SIZE)
                              << " │" << std::endl;
                    last_status_time = now;
                }
                
                // Small sleep to prevent overwhelming CPU
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
            } catch (const std::exception& e) {
                std::cerr << "❌ Signal generation error: " << e.what() << std::endl;
                break;
            }
        }
        
        std::cout << "└─────────────────────────────────────────────┘" << std::endl;
        std::cout << std::endl << "Signal generation stopped." << std::endl;
    }
    
    void stop() {
        running_ = false;
    }
    
private:
    std::vector<std::complex<int16_t>> generate_chunk() {
        std::vector<std::complex<int16_t>> chunk(BroadSpectrumConfig::CHUNK_SIZE);
        
        // Simulate multi-constellation signal generation
        for (int i = 0; i < BroadSpectrumConfig::CHUNK_SIZE; ++i) {
            double time = current_time_ + (i / sample_rate_);
            
            // GPS L1 signal (BPSK at 1575.42 MHz, offset -6.08 MHz)
            double gps_phase = 2.0 * M_PI * (6.08e6) * time;
            std::complex<int16_t> gps_signal(
                static_cast<int16_t>(1000 * BroadSpectrumConfig::GPS_WEIGHT * std::cos(gps_phase)),
                static_cast<int16_t>(1000 * BroadSpectrumConfig::GPS_WEIGHT * std::sin(gps_phase))
            );
            
            // GLONASS L1 signal (BPSK at 1602 MHz, offset +20.5 MHz)
            double glonass_phase = 2.0 * M_PI * (20.5e6) * time;
            std::complex<int16_t> glonass_signal(
                static_cast<int16_t>(800 * BroadSpectrumConfig::GLONASS_WEIGHT * std::cos(glonass_phase)),
                static_cast<int16_t>(800 * BroadSpectrumConfig::GLONASS_WEIGHT * std::sin(glonass_phase))
            );
            
            // Galileo E1 signal (BOC at 1575.42 MHz, offset -6.08 MHz)
            double galileo_phase = 2.0 * M_PI * (6.08e6) * time;
            double galileo_subcarrier = std::cos(2.0 * M_PI * 1.023e6 * time);  // BOC subcarrier
            std::complex<int16_t> galileo_signal(
                static_cast<int16_t>(900 * BroadSpectrumConfig::GALILEO_WEIGHT * std::cos(galileo_phase) * galileo_subcarrier),
                static_cast<int16_t>(900 * BroadSpectrumConfig::GALILEO_WEIGHT * std::sin(galileo_phase) * galileo_subcarrier)
            );
            
            // BeiDou B1 signal (BPSK at 1561.1 MHz, offset -20.4 MHz)
            double beidou_phase = 2.0 * M_PI * (20.4e6) * time;
            std::complex<int16_t> beidou_signal(
                static_cast<int16_t>(900 * BroadSpectrumConfig::BEIDOU_WEIGHT * std::cos(beidou_phase)),
                static_cast<int16_t>(900 * BroadSpectrumConfig::BEIDOU_WEIGHT * std::sin(beidou_phase))
            );
            
            // Sum all signals with proper weighting
            chunk[i] = gps_signal + glonass_signal + galileo_signal + beidou_signal;
        }
        
        return chunk;
    }
    
    void output_signal_to_stdout(const std::vector<std::complex<int16_t>>& signal) {
        // Output interleaved signed 16-bit IQ samples to stdout
        for (const auto& sample : signal) {
            int16_t i_sample = sample.real();
            int16_t q_sample = sample.imag();
            
            // Write directly to stdout
            std::cout.write(reinterpret_cast<const char*>(&i_sample), sizeof(int16_t));
            std::cout.write(reinterpret_cast<const char*>(&q_sample), sizeof(int16_t));
        }
        
        std::cout.flush();  // Ensure immediate output
    }
};

// Global signal generator for signal handler
GNSSSignalGenerator* generator = nullptr;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << std::endl << "Received signal " << signal << ", shutting down gracefully..." << std::endl;
    if (generator) {
        generator->stop();
    }
}

int main() {
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Create and start generator
        GNSSSignalGenerator gnss_generator;
        generator = &gnss_generator;
        
        gnss_generator.start();
        
        std::cout << std::endl << "✅ QuadGNSS Broad-Spectrum Generator completed successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}