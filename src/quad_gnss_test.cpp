#include "../include/quad_gnss_interface.h"
#include <iostream>
#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Test implementation of a dummy constellation for interface validation
namespace QuadGNSS {

class TestConstellation : public ISatelliteConstellation {
private:
    ConstellationType type_;
    double carrier_freq_;
    double frequency_offset_;
    bool ready_;
    
public:
    TestConstellation(ConstellationType type, double freq) 
        : type_(type), carrier_freq_(freq), frequency_offset_(0.0), ready_(false) {}
    
    void generate_chunk(std::complex<int16_t>* buffer, int sample_count, double time_now) override {
        // Simple test signal generation
        for (int i = 0; i < sample_count; ++i) {
            double phase = 2.0 * M_PI * frequency_offset_ * time_now;
            buffer[i] = std::complex<int16_t>(
                static_cast<int16_t>(1000 * cos(phase)),
                static_cast<int16_t>(1000 * sin(phase))
            );
        }
    }
    
    void load_ephemeris(const std::string& file_path) override {
        if (file_path.empty()) {
            throw QuadGNSSException("Empty ephemeris file path");
        }
        ready_ = true;
    }
    
    void set_frequency_offset(double offset_hz) override {
        frequency_offset_ = offset_hz;
    }
    
    ConstellationType get_constellation_type() const override {
        return type_;
    }
    
    double get_carrier_frequency() const override {
        return carrier_freq_;
    }
    
    std::vector<SatelliteInfo> get_active_satellites() const override {
        std::vector<SatelliteInfo> sats;
        if (ready_) {
            sats.emplace_back(1, type_, carrier_freq_);
            sats.emplace_back(2, type_, carrier_freq_);
        }
        return sats;
    }
    
    void configure(const GlobalConfig& config) override {
        // Configuration logic would go here
    }
    
    bool is_ready() const override {
        return ready_;
    }
};

// Test implementation for the factory
std::unique_ptr<ISatelliteConstellation> ConstellationFactory::create_constellation(ConstellationType type) {
    switch (type) {
        case ConstellationType::GPS:
            return std::make_unique<TestConstellation>(type, 1575.42e6);
        case ConstellationType::GLONASS:
            return std::make_unique<TestConstellation>(type, 1602.0e6);
        case ConstellationType::GALILEO:
            return std::make_unique<TestConstellation>(type, 1575.42e6);
        case ConstellationType::BEIDOU:
            return std::make_unique<TestConstellation>(type, 1561.098e6);
        default:
            throw QuadGNSSException("Unsupported constellation type");
    }
}

double ConstellationFactory::get_constellation_frequency(ConstellationType type) {
    switch (type) {
        case ConstellationType::GPS:
        case ConstellationType::GALILEO:
            return 1575.42e6;
        case ConstellationType::GLONASS:
            return 1602.0e6;
        case ConstellationType::BEIDOU:
            return 1561.098e6;
        default:
            throw QuadGNSSException("Unsupported constellation type");
    }
}

std::string ConstellationFactory::get_constellation_name(ConstellationType type) {
    switch (type) {
        case ConstellationType::GPS: return "GPS";
        case ConstellationType::GLONASS: return "GLONASS";
        case ConstellationType::GALILEO: return "Galileo";
        case ConstellationType::BEIDOU: return "BeiDou";
        default: return "Unknown";
    }
}

// Test implementation for SignalOrchestrator
SignalOrchestrator::SignalOrchestrator(const GlobalConfig& config) 
    : config_(config), initialized_(false) {
}

SignalOrchestrator::~SignalOrchestrator() = default;

void SignalOrchestrator::add_constellation(std::unique_ptr<ISatelliteConstellation> constellation) {
    if (!constellation) {
        throw QuadGNSSException("Cannot add null constellation");
    }
    constellations_.push_back(std::move(constellation));
}

void SignalOrchestrator::initialize(const std::map<ConstellationType, std::string>& ephemeris_file_paths) {
    if (!validate_configuration()) {
        throw QuadGNSSException("Invalid configuration");
    }
    
    // Calculate frequency offsets for frequency multiplexing
    calculate_frequency_offsets();
    
    // Load ephemeris for each constellation
    for (auto& constellation : constellations_) {
        auto type = constellation->get_constellation_type();
        auto it = ephemeris_file_paths.find(type);
        if (it != ephemeris_file_paths.end()) {
            constellation->load_ephemeris(it->second);
        }
        constellation->configure(config_);
    }
    
    initialized_ = true;
}

void SignalOrchestrator::mix_all_signals(std::complex<int16_t>* buffer, 
                                         int sample_count, 
                                         double time_now) {
    if (!initialized_ || !buffer || sample_count <= 0) {
        throw QuadGNSSException("SignalOrchestrator not properly initialized or invalid parameters");
    }
    
    // Use 32-bit accumulator to prevent overflow
    std::vector<std::complex<int32_t>> accumulator(sample_count);
    std::fill(accumulator.begin(), accumulator.end(), std::complex<int32_t>(0, 0));
    
    // Generate signals from each constellation
    for (const auto& constellation : constellations_) {
        if (constellation->is_ready()) {
            std::vector<std::complex<int16_t>> constellation_signal(sample_count);
            constellation->generate_chunk(constellation_signal.data(), sample_count, time_now);
            
            // Add to accumulator
            for (int i = 0; i < sample_count; ++i) {
                accumulator[i] += std::complex<int32_t>(
                    constellation_signal[i].real(),
                    constellation_signal[i].imag()
                );
            }
        }
    }
    
    // Prevent overflow and convert to int16_t
    prevent_overflow(accumulator.data(), sample_count);
    for (int i = 0; i < sample_count; ++i) {
        buffer[i] = std::complex<int16_t>(
            static_cast<int16_t>(accumulator[i].real()),
            static_cast<int16_t>(accumulator[i].imag())
        );
    }
}

size_t SignalOrchestrator::get_constellation_count() const {
    return constellations_.size();
}

std::vector<SatelliteInfo> SignalOrchestrator::get_all_satellites() const {
    std::vector<SatelliteInfo> all_sats;
    for (const auto& constellation : constellations_) {
        if (constellation->is_ready()) {
            auto sats = constellation->get_active_satellites();
            all_sats.insert(all_sats.end(), sats.begin(), sats.end());
        }
    }
    return all_sats;
}

bool SignalOrchestrator::is_ready() const {
    if (!initialized_) return false;
    return std::all_of(constellations_.begin(), constellations_.end(),
                       [](const auto& c) { return c->is_ready(); });
}

const GlobalConfig& SignalOrchestrator::get_config() const {
    return config_;
}

void SignalOrchestrator::calculate_frequency_offsets() {
    double center_freq = config_.center_frequency_hz;
    double min_offset = 0.0;
    
    // Find minimum frequency offset from center
    for (const auto& constellation : constellations_) {
        double carrier_freq = constellation->get_carrier_frequency();
        double offset = carrier_freq - center_freq;
        min_offset = std::min(min_offset, offset);
    }
    
    // Set frequency offsets for all constellations
    for (auto& constellation : constellations_) {
        double carrier_freq = constellation->get_carrier_frequency();
        double offset = carrier_freq - center_freq - min_offset;
        constellation->set_frequency_offset(offset);
    }
}

bool SignalOrchestrator::validate_configuration() const {
    return config_.sampling_rate_hz > 0 && 
           config_.center_frequency_hz > 0 &&
           !config_.active_constellations.empty();
}

void SignalOrchestrator::prevent_overflow(std::complex<int32_t>* accumulator, int sample_count) {
    const int32_t max_val = std::numeric_limits<int16_t>::max();
    const int32_t min_val = std::numeric_limits<int16_t>::min();
    
    for (int i = 0; i < sample_count; ++i) {
        int32_t real = accumulator[i].real();
        int32_t imag = accumulator[i].imag();
        
        // Clamp to int16_t range
        real = std::max(min_val, std::min(max_val, real));
        imag = std::max(min_val, std::min(max_val, imag));
        
        accumulator[i] = std::complex<int32_t>(real, imag);
    }
}

} // namespace QuadGNSS