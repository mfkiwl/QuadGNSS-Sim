#include "../include/quad_gnss_interface.h"
#include "../include/rinex_parser.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace QuadGNSS {

// Helper class for NCO (Numerically Controlled Oscillator)
class DigitalNCO {
private:
    double sample_rate_hz_;
    double frequency_hz_;
    double phase_;
    double phase_increment_;
    
    // Precomputed sine/cosine lookup table for efficiency
    static constexpr size_t LUT_SIZE = 16384;
    std::vector<std::complex<float>> lookup_table_;
    
public:
    DigitalNCO(double sample_rate_hz) 
        : sample_rate_hz_(sample_rate_hz)
        , frequency_hz_(0.0)
        , phase_(0.0)
        , phase_increment_(0.0)
        , lookup_table_(LUT_SIZE) {
        
        // Precompute sine/cosine lookup table
        for (size_t i = 0; i < LUT_SIZE; ++i) {
            double angle = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(LUT_SIZE);
            lookup_table_[i] = std::complex<float>(
                static_cast<float>(std::cos(angle)),
                static_cast<float>(std::sin(angle))
            );
        }
    }
    
    void set_frequency(double frequency_hz) {
        frequency_hz_ = frequency_hz;
        phase_increment_ = 2.0 * M_PI * frequency_hz_ / sample_rate_hz_;
    }
    
    void reset_phase() {
        phase_ = 0.0;
    }
    
    // Generate complex carrier samples
    void generate_samples(std::complex<float>* buffer, int count) {
        for (int i = 0; i < count; ++i) {
            // Convert phase to table index
            size_t index = static_cast<size_t>(phase_ * LUT_SIZE / (2.0 * M_PI)) % LUT_SIZE;
            buffer[i] = lookup_table_[index];
            
            // Advance phase
            phase_ += phase_increment_;
            if (phase_ >= 2.0 * M_PI) {
                phase_ -= 2.0 * M_PI;
            }
        }
    }
    
    // Mix signal with carrier (complex multiplication)
    void mix_signal(const std::complex<int16_t>* input, 
                   std::complex<int16_t>* output, 
                   int count) {
        
        std::vector<std::complex<float>> carrier(count);
        generate_samples(carrier.data(), count);
        
        for (int i = 0; i < count; ++i) {
            // Convert int16_t to float for multiplication
            std::complex<float> signal_float(
                static_cast<float>(input[i].real()),
                static_cast<float>(input[i].imag())
            );
            
            // Complex multiplication with carrier
            std::complex<float> mixed = signal_float * carrier[i];
            
            // Convert back to int16_t with scaling to prevent overflow
            constexpr float SCALE_FACTOR = 0.5f; // Adjust as needed
            output[i] = std::complex<int16_t>(
                static_cast<int16_t>(mixed.real() * SCALE_FACTOR),
                static_cast<int16_t>(mixed.imag() * SCALE_FACTOR)
            );
        }
    }
};

// Base class for CDMA providers with common functionality
class CDMAProviderBase : public ISatelliteConstellation {
protected:
    ConstellationType constellation_type_;
    double carrier_frequency_hz_;
    double frequency_offset_hz_;
    bool configured_;
    bool ephemeris_loaded_;
    
    DigitalNCO nco_;
    GlobalConfig config_;
    
    // Satellite configuration
    struct SatelliteConfig {
        int prn;
        double doppler_hz;
        double power_dbm;
        double code_phase_chips;
        double carrier_phase_rad;
        bool is_active;
        EphemerisData ephemeris;  // Loaded ephemeris data
    };
    
    std::vector<SatelliteConfig> active_satellites_;
    
public:
    CDMAProviderBase(ConstellationType type, double carrier_freq_hz)
        : constellation_type_(type)
        , carrier_frequency_hz_(carrier_freq_hz)
        , frequency_offset_hz_(0.0)
        , configured_(false)
        , ephemeris_loaded_(false)
        , nco_(GlobalConfig::DEFAULT_SAMPLING_RATE) {
    }
    
    // Pure virtual interface implementations
    ConstellationType get_constellation_type() const override {
        return constellation_type_;
    }
    
    double get_carrier_frequency() const override {
        return carrier_frequency_hz_;
    }
    
    void set_frequency_offset(double offset_hz) override {
        frequency_offset_hz_ = offset_hz;
        nco_.set_frequency(offset_hz);
    }
    
    std::vector<SatelliteInfo> get_active_satellites() const override {
        std::vector<SatelliteInfo> info;
        for (const auto& sat : active_satellites_) {
            if (sat.is_active) {
                SatelliteInfo sat_info;
                sat_info.prn = sat.prn;
                sat_info.constellation = constellation_type_;
                sat_info.frequency_hz = carrier_frequency_hz_ + frequency_offset_hz_;
                sat_info.power_dbm = sat.power_dbm;
                sat_info.doppler_hz = sat.doppler_hz;
                sat_info.is_active = sat.is_active;
                info.push_back(sat_info);
            }
        }
        return info;
    }
    
    void configure(const GlobalConfig& config) override {
        config_ = config;
        
        // Initialize default satellite configuration
        initialize_default_satellites();
        
        configured_ = true;
    }
    
    bool is_ready() const override {
        return configured_ && ephemeris_loaded_ && !active_satellites_.empty();
    }
    
protected:
    virtual void initialize_default_satellites() = 0;
    
    // Helper method to calculate frequency offset from center frequency
    double calculate_frequency_offset(double center_freq_hz) const {
        return carrier_frequency_hz_ - center_freq_hz;
    }
    
    // Simple satellite position calculation from ephemeris
    struct SatellitePosition {
        double x, y, z;        // ECEF coordinates (m)
        double range;           // Range to user (m)
        double doppler;         // Doppler shift (Hz)
    };
    
    SatellitePosition calculate_satellite_position(const EphemerisData& eph, double time_sec) const {
        SatellitePosition pos = {0, 0, 0, 0, 0};
        
        if (!eph.is_valid) return pos;
        
        // Simplified satellite position calculation
        // This is a basic implementation - real GNSS would use more precise algorithms
        
        const double mu = 3.986005e14;  // Earth's gravitational parameter (m^3/s^2)
        const double omega_e = 7.2921151467e-5;  // Earth's rotation rate (rad/s)
        
        // Time from ephemeris reference
        double dt = time_sec - eph.toe;
        double a = eph.sqrt_a * eph.sqrt_a;  // Semi-major axis
        
        // Mean motion
        double n0 = std::sqrt(mu / (a * a * a));
        double n = n0 + eph.delta_n;
        
        // Mean anomaly
        double M = eph.m0 + n * dt;
        
        // Solve Kepler's equation (simplified)
        double E = M;  // Eccentric anomaly
        for (int i = 0; i < 10; ++i) {
            E = M + eph.e * std::sin(E);
        }
        
        // True anomaly
        double nu = 2.0 * std::atan(std::sqrt((1.0 + eph.e) / (1.0 - eph.e)) * std::tan(E / 2.0));
        
        // Radius
        double r = a * (1.0 - eph.e * std::cos(E));
        
        // Position in orbital plane
        double x_orb = r * std::cos(nu);
        double y_orb = r * std::sin(nu);
        
        // Corrected arguments
        double u = nu + eph.cuc * std::cos(2.0 * nu) + eph.cus * std::sin(2.0 * nu);
        double r_corr = r + eph.crc * std::cos(2.0 * nu) + eph.crs * std::sin(2.0 * nu);
        double i_corr = eph.i0 + eph.idot * dt + eph.cic * std::cos(2.0 * nu) + eph.cis * std::sin(2.0 * nu);
        
        // Corrected longitude of ascending node
        double omega_corr = eph.omega0 + (eph.omega_dot - omega_e) * dt;
        
        // ECEF coordinates (simplified)
        pos.x = r_corr * (std::cos(u) * std::cos(omega_corr) - std::sin(u) * std::cos(i_corr) * std::sin(omega_corr));
        pos.y = r_corr * (std::cos(u) * std::sin(omega_corr) + std::sin(u) * std::cos(i_corr) * std::cos(omega_corr));
        pos.z = r_corr * std::sin(u) * std::sin(i_corr);
        
        // Simplified range and Doppler calculation
        // Assuming user at origin for now
        pos.range = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        
        // Relative velocity for Doppler (simplified)
        double v_rel = std::sqrt(mu / (a * a * a)) * eph.sqrt_a * eph.e * std::sin(nu);
        pos.doppler = -v_rel * 1575.42e6 / 299792458.0;  // GPS carrier frequency / speed of light
        
        return pos;
    }
};

// GPS L1 C/A Provider
class GpsL1Provider : public CDMAProviderBase {
private:
    // GPS C/A Code generation state (persistent across chunks)
    struct GPSCodeState {
        // G1 LFSR: x^10 + x^3 + 1
        unsigned int g1_register;  // 10-bit register
        // G2 LFSR: x^10 + x^9 + x^8 + x^6 + x^3 + x^2 + 1  
        unsigned int g2_register;  // 10-bit register
        int chip_count;          // Current position in 1023-chip sequence
        
        GPSCodeState() : g1_register(0x3FF), g2_register(0x3FF), chip_count(0) {}
    };
    
    // Tap delay table for GPS PRNs 1-37 (from ICD-GPS-200)
    static const int gps_tap_delays[37][2];
    
    std::map<int, GPSCodeState> code_states_;  // Per-satellite state
    std::map<int, EphemerisData> ephemeris_data_;  // Loaded ephemeris data
    
public:
    GpsL1Provider() : CDMAProviderBase(ConstellationType::GPS, 1575.42e6) {
        // GPS L1 C/A specific parameters
    }
    
    void load_ephemeris(const std::string& file_path) override {
        try {
            std::cout << "GPS L1: Loading ephemeris from " << file_path << std::endl;
            
            // Parse RINEX 2.11 GPS ephemeris file
            ephemeris_data_ = RINEXParser::parse_gps_rinex2(file_path);
            
            // Update active satellites with loaded ephemeris data
            for (auto& sat : active_satellites_) {
                if (ephemeris_data_.find(sat.prn) != ephemeris_data_.end()) {
                    sat.ephemeris = ephemeris_data_[sat.prn];
                    std::cout << "  Loaded GPS PRN " << sat.prn 
                              << " ephemeris (valid: " << (sat.ephemeris.is_valid ? "yes" : "no") << ")" << std::endl;
                } else {
                    std::cout << "  No ephemeris found for GPS PRN " << sat.prn << std::endl;
                }
            }
            
            ephemeris_loaded_ = true;
            std::cout << "GPS L1: Successfully loaded " << ephemeris_data_.size() << " ephemeris records" << std::endl;
            
        } catch (const std::exception& e) {
            throw QuadGNSSException("Failed to load GPS ephemeris: " + std::string(e.what()));
        }
    }
    
    void generate_chunk(std::complex<int16_t>* buffer, int sample_count, double time_now) override {
        if (!is_ready()) {
            throw QuadGNSSException("GPS L1 Provider not ready for signal generation");
        }
        
        // Clear output buffer
        std::fill(buffer, buffer + sample_count, std::complex<int16_t>(0, 0));
        
        // GPS signal parameters
        const double chip_rate = 1.023e6;  // GPS L1 C/A chip rate
        const double carrier_freq = 1575.42e6;  // GPS L1 carrier frequency
        const double samples_per_chip = config_.sampling_rate_hz / chip_rate;
        
        // Generate signals for each active GPS satellite
        for (auto& sat : active_satellites_) {
            if (!sat.is_active) continue;
            
            // Initialize code state if not exists
            if (code_states_.find(sat.prn) == code_states_.end()) {
                code_states_[sat.prn] = GPSCodeState();
                // Set initial G2 state based on PRN tap delay
                if (sat.prn >= 1 && sat.prn <= 37) {
                    int delay = gps_tap_delays[sat.prn-1][1];
                    unsigned int g2_init = 0x3FF;
                    for (int i = 0; i < delay; ++i) {
                        // Advance G2 register
                        unsigned int feedback = ((g2_init >> 2) & 1) ^ ((g2_init >> 9) & 1) ^ 
                                            ((g2_init >> 8) & 1) ^ ((g2_init >> 6) & 1) ^ 
                                            ((g2_init >> 3) & 1) ^ ((g2_init >> 1) & 1) ^ ((g2_init >> 0) & 1);
                        g2_init = ((feedback & 1) << 9) | (g2_init >> 1);
                    }
                    code_states_[sat.prn].g2_register = g2_init;
                }
            }
            
            GPSCodeState& state = code_states_[sat.prn];
            
            // Calculate satellite position and Doppler from ephemeris
            SatellitePosition sat_pos = {0, 0, 0, 0, 0};
            if (sat.ephemeris.is_valid) {
                sat_pos = calculate_satellite_position(sat.ephemeris, time_now);
                sat.doppler_hz = sat_pos.doppler;
            }
            
            // Generate GPS L1 C/A spread spectrum signal
            for (int i = 0; i < sample_count; ++i) {
                double time = time_now + static_cast<double>(i) / config_.sampling_rate_hz;
                
                // Calculate chip position
                double chip_position = time * chip_rate;
                int chip_index = static_cast<int>(chip_position) % 1023;
                
                // Generate Gold code chip if at new position
                if (chip_index != state.chip_count) {
                    state.chip_count = chip_index;
                    
                    // G1 LFSR feedback: x^10 + x^3 + 1
                    unsigned int g1_feedback = ((state.g1_register >> 2) & 1) ^ ((state.g1_register >> 9) & 1);
                    
                    // G2 LFSR feedback: x^10 + x^9 + x^8 + x^6 + x^3 + x^2 + 1
                    unsigned int g2_feedback = ((state.g2_register >> 2) & 1) ^ ((state.g2_register >> 9) & 1) ^ 
                                            ((state.g2_register >> 8) & 1) ^ ((state.g2_register >> 6) & 1) ^ 
                                            ((state.g2_register >> 3) & 1) ^ ((state.g2_register >> 1) & 1) ^ 
                                            ((state.g2_register >> 0) & 1);
                    
                    // Get output chips
                    int g1_chip = (state.g1_register >> 9) & 1;
                    int g2_chip = (state.g2_register >> 9) & 1;
                    
                    // Apply G2 tap delay for this PRN
                    int tap_delay = gps_tap_delays[sat.prn-1][1];
                    unsigned int g2_delayed = state.g2_register;
                    for (int d = 0; d < tap_delay; ++d) {
                        unsigned int feedback = ((g2_delayed >> 2) & 1) ^ ((g2_delayed >> 9) & 1) ^ 
                                            ((g2_delayed >> 8) & 1) ^ ((g2_delayed >> 6) & 1) ^ 
                                            ((g2_delayed >> 3) & 1) ^ ((g2_delayed >> 1) & 1) ^ 
                                            ((g2_delayed >> 0) & 1);
                        g2_delayed = ((feedback & 1) << 9) | (g2_delayed >> 1);
                    }
                    int g2_delayed_chip = (g2_delayed >> 9) & 1;
                    
                    // Gold code chip = G1 XOR G2_delayed
                    int gold_chip = g1_chip ^ g2_delayed_chip;
                    
                    // Store current chip value for this sample
                    sat.code_phase_chips = gold_chip;
                    
                    // Advance registers for next chip
                    state.g1_register = ((g1_feedback & 1) << 9) | (state.g1_register >> 1);
                    state.g2_register = ((g2_feedback & 1) << 9) | (state.g2_register >> 1);
                }
                
                // Convert chip to BPSK signal (+1/-1)
                int chip_value = (sat.code_phase_chips == 1) ? 1 : -1;
                
                // Apply carrier modulation (BPSK at carrier frequency with Doppler)
                double carrier_phase = 2.0 * M_PI * (carrier_freq + sat.doppler_hz) * time + sat.carrier_phase_rad;
                double carrier = std::cos(carrier_phase);
                
                // Generate signal sample
                double signal_value = chip_value * carrier * 1000.0;  // Scale for int16 range
                
                // Convert to complex (I-only for BPSK)
                std::complex<int16_t> sample(
                    static_cast<int16_t>(signal_value),
                    0
                );
                
                // Add to main buffer with overflow protection
                if (i < sample_count) {  // Buffer saturation check
                    int32_t new_i = static_cast<int32_t>(buffer[i].real()) + static_cast<int32_t>(sample.real());
                    int32_t new_q = static_cast<int32_t>(buffer[i].imag()) + static_cast<int32_t>(sample.imag());
                    
                    // Clamp to int16 range to prevent overflow
                    buffer[i] = std::complex<int16_t>(
                        static_cast<int16_t>(std::max(-32768, std::min(32767, new_i))),
                        static_cast<int16_t>(std::max(-32768, std::min(32767, new_q)))
                    );
                }
            }
            
            // Update carrier phase for continuity
            sat.carrier_phase_rad += 2.0 * M_PI * (carrier_freq + sat.doppler_hz) * sample_count / config_.sampling_rate_hz;
            sat.carrier_phase_rad = fmod(sat.carrier_phase_rad, 2.0 * M_PI);
        }
        
        // Apply frequency offset using digital mixing (NCO)
        if (std::abs(frequency_offset_hz_) > 1.0) {  // Only mix if significant offset
            std::vector<std::complex<int16_t>> mixed_signal(sample_count);
            nco_.mix_signal(buffer, mixed_signal.data(), sample_count);
            std::copy(mixed_signal.begin(), mixed_signal.end(), buffer);
        }
    }
    
private:
    void initialize_default_satellites() override {
        active_satellites_.clear();
        
        // Initialize default GPS satellites (PRN 1-32)
        for (int prn = 1; prn <= 32; ++prn) {
            SatelliteConfig sat;
            sat.prn = prn;
            sat.doppler_hz = 0.0;  // Will be calculated based on user position
            sat.power_dbm = -130.0;  // Typical GPS signal power
            sat.code_phase_chips = 0.0;
            sat.carrier_phase_rad = 0.0;
            sat.is_active = (prn <= 8);  // Activate first 8 satellites by default
            active_satellites_.push_back(sat);
        }
    }
    
    void generate_gps_signal_placeholder(std::complex<int16_t>* buffer, 
                                         int sample_count, 
                                         double time_now, 
                                         SatelliteConfig& sat) {
        // This function is no longer used - replaced by real implementation
        std::fill(buffer, buffer + sample_count, std::complex<int16_t>(0, 0));
    }
};

// GPS C/A Code Tap Delay Table (PRN 1-37)
const int GpsL1Provider::gps_tap_delays[37][2] = {
    {2, 6},   {3, 7},   {4, 8},   {5, 9},   {1, 9},   {2, 10},  {1, 8},   {2, 9},
    {3, 10},  {2, 3},   {3, 4},   {5, 6},   {6, 7},   {7, 8},   {8, 9},   {9, 10},
    {1, 4},   {2, 5},   {3, 6},   {4, 7},   {5, 8},   {6, 9},   {1, 3},   {4, 6},
    {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 6},   {2, 7},   {3, 8},   {4, 9},
    {5, 10},  {4, 10},  {1, 7},   {2, 8},   {4, 10}
};

// Galileo E1 OS Provider
class GalileoE1Provider : public CDMAProviderBase {
private:
    // Galileo E1 Code generation state (persistent across chunks)
    struct GalileoCodeState {
        // Galileo E1 uses tiered codes with primary and secondary sequences
        unsigned int primary_lfsr;     // Primary code LFSR (4092 chips)
        unsigned int secondary_lfsr;     // Secondary code LFSR (25 chips)
        int primary_chip_count;         // Current position in primary code
        int secondary_chip_count;       // Current position in secondary code
        int boc_phase;                 // BOC subcarrier phase
        
        GalileoCodeState() : primary_lfsr(0xFFF), secondary_lfsr(0x1F), 
                           primary_chip_count(0), secondary_chip_count(0), boc_phase(0) {}
    };
    
    // Galileo E1 tiered code polynomials (from Galileo ICD)
    static const unsigned int GALILEO_PRIMARY_POLY = 0x921;  // Primary code polynomial
    static const unsigned int GALILEO_SECONDARY_POLY = 0x12;  // Secondary code polynomial (25 chips)
    
    std::map<int, GalileoCodeState> code_states_;  // Per-satellite state
    std::map<int, EphemerisData> ephemeris_data_;  // Loaded ephemeris data
    
public:
    GalileoE1Provider() : CDMAProviderBase(ConstellationType::GALILEO, 1575.42e6) {
        // Galileo E1 OS specific parameters
    }
    
    void load_ephemeris(const std::string& file_path) override {
        try {
            std::cout << "Galileo E1: Loading ephemeris from " << file_path << std::endl;
            
            // Parse RINEX 3.0 Galileo ephemeris file
            ephemeris_data_ = RINEXParser::parse_rinex3(file_path, ConstellationType::GALILEO);
            
            // Update active satellites with loaded ephemeris data
            for (auto& sat : active_satellites_) {
                if (ephemeris_data_.find(sat.prn) != ephemeris_data_.end()) {
                    sat.ephemeris = ephemeris_data_[sat.prn];
                    std::cout << "  Loaded Galileo PRN " << sat.prn 
                              << " ephemeris (valid: " << (sat.ephemeris.is_valid ? "yes" : "no") << ")" << std::endl;
                } else {
                    std::cout << "  No ephemeris found for Galileo PRN " << sat.prn << std::endl;
                }
            }
            
            ephemeris_loaded_ = true;
            std::cout << "Galileo E1: Successfully loaded " << ephemeris_data_.size() << " ephemeris records" << std::endl;
            
        } catch (const std::exception& e) {
            throw QuadGNSSException("Failed to load Galileo ephemeris: " + std::string(e.what()));
        }
    }
    
    void generate_chunk(std::complex<int16_t>* buffer, int sample_count, double time_now) override {
        if (!is_ready()) {
            throw QuadGNSSException("Galileo E1 Provider not ready for signal generation");
        }
        
        std::fill(buffer, buffer + sample_count, std::complex<int16_t>(0, 0));
        
        // Galileo E1 signal parameters
        const double chip_rate = 1.023e6;  // Galileo E1 chip rate (same as GPS)
        const double carrier_freq = 1575.42e6;  // Galileo E1 carrier frequency
        const double boc_subcarrier_rate = 1.023e6;  // BOC(1,1) subcarrier frequency
        const double samples_per_chip = config_.sampling_rate_hz / chip_rate;
        
        // Generate signals for each active Galileo satellite
        for (auto& sat : active_satellites_) {
            if (!sat.is_active) continue;
            
            // Initialize code state if not exists
            if (code_states_.find(sat.prn) == code_states_.end()) {
                code_states_[sat.prn] = GalileoCodeState();
                // Set initial registers based on PRN
                unsigned int primary_seed = 0x800 + ((sat.prn * 13) & 0xFFF);  // PRN-based seed
                unsigned int secondary_seed = 0x10 + ((sat.prn * 3) & 0x1F);
                code_states_[sat.prn].primary_lfsr = primary_seed & 0xFFF;
                code_states_[sat.prn].secondary_lfsr = secondary_seed & 0x1F;
            }
            
            GalileoCodeState& state = code_states_[sat.prn];
            
            // Generate Galileo E1 OS spread spectrum signal with BOC(1,1) modulation
            for (int i = 0; i < sample_count; ++i) {
                double time = time_now + static_cast<double>(i) / config_.sampling_rate_hz;
                
                // Calculate chip position
                double chip_position = time * chip_rate;
                int primary_chip_index = static_cast<int>(chip_position) % 4092;
                int secondary_chip_index = (primary_chip_index / 4092) % 25;  // Secondary repeats every 25 primary chips
                
                // Generate tiered code chip if at new position
                if (primary_chip_index != state.primary_chip_count) {
                    state.primary_chip_count = primary_chip_index;
                    state.secondary_chip_count = secondary_chip_index;
                    
                    // Primary code LFSR feedback (12-bit LFSR)
                    unsigned int primary_feedback = ((state.primary_lfsr >> 0) & 1) ^ 
                                                 ((state.primary_lfsr >> 2) & 1) ^ 
                                                 ((state.primary_lfsr >> 3) & 1) ^ 
                                                 ((state.primary_lfsr >> 5) & 1) ^ 
                                                 ((state.primary_lfsr >> 6) & 1) ^ 
                                                 ((state.primary_lfsr >> 9) & 1) ^ 
                                                 ((state.primary_lfsr >> 10) & 1) ^ 
                                                 ((state.primary_lfsr >> 11) & 1);
                    
                    // Secondary code LFSR feedback (5-bit LFSR)
                    unsigned int secondary_feedback = ((state.secondary_lfsr >> 2) & 1) ^ 
                                                  ((state.secondary_lfsr >> 4) & 1);
                    
                    // Get output chips
                    int primary_chip = (state.primary_lfsr >> 11) & 1;
                    int secondary_chip = (state.secondary_lfsr >> 4) & 1;
                    
                    // Tiered code: Primary XOR Secondary
                    int tiered_chip = primary_chip ^ secondary_chip;
                    
                    // Store current chip value for this sample
                    sat.code_phase_chips = tiered_chip;
                    
                    // Advance registers for next chip
                    state.primary_lfsr = ((primary_feedback & 1) << 11) | (state.primary_lfsr >> 1);
                    state.secondary_lfsr = ((secondary_feedback & 1) << 4) | (state.secondary_lfsr >> 1);
                }
                
                // Convert tiered code to BPSK signal (+1/-1)
                int chip_value = (sat.code_phase_chips == 1) ? 1 : -1;
                
                // Generate BOC(1,1) subcarrier
                double boc_phase = 2.0 * M_PI * boc_subcarrier_rate * time + state.boc_phase;
                double boc_subcarrier = std::cos(boc_phase);  // BOC(1,1) uses cosine
                
                // Apply BOC modulation: multiply code by subcarrier
                int boc_modulated_chip = chip_value * ((boc_subcarrier > 0) ? 1 : -1);
                
                // Apply carrier modulation (BOC-modulated BPSK at carrier frequency with Doppler)
                double carrier_phase = 2.0 * M_PI * (carrier_freq + sat.doppler_hz) * time + sat.carrier_phase_rad;
                double carrier = std::cos(carrier_phase);
                
                // Generate signal sample
                double signal_value = boc_modulated_chip * carrier * 800.0;  // Scale for int16 range (BOC typically lower power)
                
                // Convert to complex (I-only for BOC-BPSK)
                std::complex<int16_t> sample(
                    static_cast<int16_t>(signal_value),
                    0
                );
                
                // Add to main buffer with overflow protection
                if (i < sample_count) {  // Buffer saturation check
                    int32_t new_i = static_cast<int32_t>(buffer[i].real()) + static_cast<int32_t>(sample.real());
                    int32_t new_q = static_cast<int32_t>(buffer[i].imag()) + static_cast<int32_t>(sample.imag());
                    
                    // Clamp to int16 range to prevent overflow
                    buffer[i] = std::complex<int16_t>(
                        static_cast<int16_t>(std::max(-32768, std::min(32767, new_i))),
                        static_cast<int16_t>(std::max(-32768, std::min(32767, new_q)))
                    );
                }
            }
            
            // Update carrier and BOC phases for continuity
            sat.carrier_phase_rad += 2.0 * M_PI * (carrier_freq + sat.doppler_hz) * sample_count / config_.sampling_rate_hz;
            sat.carrier_phase_rad = fmod(sat.carrier_phase_rad, 2.0 * M_PI);
            
            state.boc_phase += 2.0 * M_PI * boc_subcarrier_rate * sample_count / config_.sampling_rate_hz;
            state.boc_phase = fmod(state.boc_phase, 2.0 * M_PI);
        }
        
        // Apply frequency offset using digital mixing
        if (std::abs(frequency_offset_hz_) > 1.0) {
            std::vector<std::complex<int16_t>> mixed_signal(sample_count);
            nco_.mix_signal(buffer, mixed_signal.data(), sample_count);
            std::copy(mixed_signal.begin(), mixed_signal.end(), buffer);
        }
    }
    
private:
    void initialize_default_satellites() override {
        active_satellites_.clear();
        
        // Initialize default Galileo satellites (PRN 1-36)
        for (int prn = 1; prn <= 36; ++prn) {
            SatelliteConfig sat;
            sat.prn = prn;
            sat.doppler_hz = 0.0;
            sat.power_dbm = -127.0;  // Typical Galileo signal power
            sat.code_phase_chips = 0.0;
            sat.carrier_phase_rad = 0.0;
            sat.is_active = (prn <= 6);  // Activate first 6 satellites by default
            active_satellites_.push_back(sat);
        }
    }
    
    void generate_galileo_signal_placeholder(std::complex<int16_t>* buffer, 
                                            int sample_count, 
                                            double time_now, 
                                            SatelliteConfig& sat) {
        // This function is no longer used - replaced by real implementation
        std::fill(buffer, buffer + sample_count, std::complex<int16_t>(0, 0));
    }
};

// Beidou B1I Provider
class BeidouB1Provider : public CDMAProviderBase {
private:
    // BeiDou B1I Code generation state (persistent across chunks)
    struct BeidouCodeState {
        // BeiDou B1I uses 11-bit linear shift registers
        unsigned int lfsr1_register;  // First 11-bit LFSR
        unsigned int lfsr2_register;  // Second 11-bit LFSR
        int chip_count;               // Current position in 2046-chip sequence
        
        BeidouCodeState() : lfsr1_register(0x7FF), lfsr2_register(0x7FF), chip_count(0) {}
    };
    
    // BeiDou B1I polynomial coefficients (from BDS-SIS-ICD-B1I)
    static const unsigned int BEIDOU_POLY1 = 0x405;  // x^11 + x^9 + x^8 + x^5 + x^2 + 1
    static const unsigned int BEIDOU_POLY2 = 0x679;  // x^11 + x^10 + x^8 + x^7 + x^4 + x^3 + 1
    
    std::map<int, BeidouCodeState> code_states_;  // Per-satellite state
    std::map<int, EphemerisData> ephemeris_data_;  // Loaded ephemeris data
    
public:
    BeidouB1Provider() : CDMAProviderBase(ConstellationType::BEIDOU, 1561.098e6) {
        // Beidou B1I specific parameters
    }
    
    void load_ephemeris(const std::string& file_path) override {
        try {
            std::cout << "BeiDou B1I: Loading ephemeris from " << file_path << std::endl;
            
            // Parse RINEX 3.0 BeiDou ephemeris file
            ephemeris_data_ = RINEXParser::parse_rinex3(file_path, ConstellationType::BEIDOU);
            
            // Update active satellites with loaded ephemeris data
            for (auto& sat : active_satellites_) {
                if (ephemeris_data_.find(sat.prn) != ephemeris_data_.end()) {
                    sat.ephemeris = ephemeris_data_[sat.prn];
                    std::cout << "  Loaded BeiDou PRN " << sat.prn 
                              << " ephemeris (valid: " << (sat.ephemeris.is_valid ? "yes" : "no") << ")" << std::endl;
                } else {
                    std::cout << "  No ephemeris found for BeiDou PRN " << sat.prn << std::endl;
                }
            }
            
            ephemeris_loaded_ = true;
            std::cout << "BeiDou B1I: Successfully loaded " << ephemeris_data_.size() << " ephemeris records" << std::endl;
            
        } catch (const std::exception& e) {
            throw QuadGNSSException("Failed to load BeiDou ephemeris: " + std::string(e.what()));
        }
    }
    
    void generate_chunk(std::complex<int16_t>* buffer, int sample_count, double time_now) override {
        if (!is_ready()) {
            throw QuadGNSSException("Beidou B1 Provider not ready for signal generation");
        }
        
        std::fill(buffer, buffer + sample_count, std::complex<int16_t>(0, 0));
        
        // BeiDou B1I signal parameters
        const double chip_rate = 2.046e6;  // BeiDou B1I chip rate (2x GPS)
        const double carrier_freq = 1561.098e6;  // BeiDou B1I carrier frequency
        const double samples_per_chip = config_.sampling_rate_hz / chip_rate;
        
        // Generate signals for each active Beidou satellite
        for (auto& sat : active_satellites_) {
            if (!sat.is_active) continue;
            
            // Initialize code state if not exists
            if (code_states_.find(sat.prn) == code_states_.end()) {
                code_states_[sat.prn] = BeidouCodeState();
                // Set initial registers based on PRN (different seeds for different satellites)
                unsigned int seed1 = 0x400 + (sat.prn & 0x3FF);  // PRN-based seed
                unsigned int seed2 = 0x600 + ((sat.prn * 7) & 0x3FF);
                code_states_[sat.prn].lfsr1_register = seed1 & 0x7FF;
                code_states_[sat.prn].lfsr2_register = seed2 & 0x7FF;
            }
            
            BeidouCodeState& state = code_states_[sat.prn];
            
            // Generate BeiDou B1I spread spectrum signal
            for (int i = 0; i < sample_count; ++i) {
                double time = time_now + static_cast<double>(i) / config_.sampling_rate_hz;
                
                // Calculate chip position
                double chip_position = time * chip_rate;
                int chip_index = static_cast<int>(chip_position) % 2046;
                
                // Generate CSS code chip if at new position
                if (chip_index != state.chip_count) {
                    state.chip_count = chip_index;
                    
                    // LFSR1 feedback for polynomial x^11 + x^9 + x^8 + x^5 + x^2 + 1
                    unsigned int lfsr1_feedback = ((state.lfsr1_register >> 2) & 1) ^ 
                                                ((state.lfsr1_register >> 5) & 1) ^ 
                                                ((state.lfsr1_register >> 8) & 1) ^ 
                                                ((state.lfsr1_register >> 9) & 1) ^ 
                                                ((state.lfsr1_register >> 10) & 1);
                    
                    // LFSR2 feedback for polynomial x^11 + x^10 + x^8 + x^7 + x^4 + x^3 + 1
                    unsigned int lfsr2_feedback = ((state.lfsr2_register >> 3) & 1) ^ 
                                                ((state.lfsr2_register >> 4) & 1) ^ 
                                                ((state.lfsr2_register >> 7) & 1) ^ 
                                                ((state.lfsr2_register >> 8) & 1) ^ 
                                                ((state.lfsr2_register >> 9) & 1) ^ 
                                                ((state.lfsr2_register >> 10) & 1);
                    
                    // Get output chips from LFSRs
                    int lfsr1_chip = (state.lfsr1_register >> 10) & 1;
                    int lfsr2_chip = (state.lfsr2_register >> 10) & 1;
                    
                    // CSS (Chip Sequence Selection) - combine LFSRs
                    // Different combination for each PRN to create unique sequences
                    int css_chip;
                    switch (sat.prn % 4) {
                        case 0: css_chip = lfsr1_chip ^ lfsr2_chip; break;
                        case 1: css_chip = lfsr1_chip & (~lfsr2_chip & 1); break;
                        case 2: css_chip = (~lfsr1_chip & 1) ^ lfsr2_chip; break;
                        case 3: css_chip = lfsr1_chip; break;
                        default: css_chip = lfsr1_chip ^ lfsr2_chip; break;
                    }
                    
                    // Store current chip value for this sample
                    sat.code_phase_chips = css_chip;
                    
                    // Advance registers for next chip
                    state.lfsr1_register = ((lfsr1_feedback & 1) << 10) | (state.lfsr1_register >> 1);
                    state.lfsr2_register = ((lfsr2_feedback & 1) << 10) | (state.lfsr2_register >> 1);
                }
                
                // Convert chip to BPSK signal (+1/-1)
                int chip_value = (sat.code_phase_chips == 1) ? 1 : -1;
                
                // Apply carrier modulation (BPSK at carrier frequency with Doppler)
                double carrier_phase = 2.0 * M_PI * (carrier_freq + sat.doppler_hz) * time + sat.carrier_phase_rad;
                double carrier = std::cos(carrier_phase);
                
                // Generate signal sample
                double signal_value = chip_value * carrier * 900.0;  // Scale for int16 range (slightly lower power)
                
                // Convert to complex (I-only for BPSK)
                std::complex<int16_t> sample(
                    static_cast<int16_t>(signal_value),
                    0
                );
                
                // Add to main buffer with overflow protection
                if (i < sample_count) {  // Buffer saturation check
                    int32_t new_i = static_cast<int32_t>(buffer[i].real()) + static_cast<int32_t>(sample.real());
                    int32_t new_q = static_cast<int32_t>(buffer[i].imag()) + static_cast<int32_t>(sample.imag());
                    
                    // Clamp to int16 range to prevent overflow
                    buffer[i] = std::complex<int16_t>(
                        static_cast<int16_t>(std::max(-32768, std::min(32767, new_i))),
                        static_cast<int16_t>(std::max(-32768, std::min(32767, new_q)))
                    );
                }
            }
            
            // Update carrier phase for continuity
            sat.carrier_phase_rad += 2.0 * M_PI * (carrier_freq + sat.doppler_hz) * sample_count / config_.sampling_rate_hz;
            sat.carrier_phase_rad = fmod(sat.carrier_phase_rad, 2.0 * M_PI);
        }
        
        // Apply frequency offset using digital mixing
        if (std::abs(frequency_offset_hz_) > 1.0) {
            std::vector<std::complex<int16_t>> mixed_signal(sample_count);
            nco_.mix_signal(buffer, mixed_signal.data(), sample_count);
            std::copy(mixed_signal.begin(), mixed_signal.end(), buffer);
        }
    }
    
private:
    void initialize_default_satellites() override {
        active_satellites_.clear();
        
        // Initialize default Beidou satellites (PRN 1-37)
        for (int prn = 1; prn <= 37; ++prn) {
            SatelliteConfig sat;
            sat.prn = prn;
            sat.doppler_hz = 0.0;
            sat.power_dbm = -133.0;  // Typical Beidou signal power
            sat.code_phase_chips = 0.0;
            sat.carrier_phase_rad = 0.0;
            sat.is_active = (prn <= 5);  // Activate first 5 satellites by default
            active_satellites_.push_back(sat);
        }
    }
    
    void generate_beidou_signal_placeholder(std::complex<int16_t>* buffer, 
                                             int sample_count, 
                                             double time_now, 
                                             SatelliteConfig& sat) {
        // This function is no longer used - replaced by real implementation
        std::fill(buffer, buffer + sample_count, std::complex<int16_t>(0, 0));
    }
};

// Update the factory to create these new providers
std::unique_ptr<ISatelliteConstellation> ConstellationFactory::create_constellation(ConstellationType type) {
    switch (type) {
        case ConstellationType::GPS:
            return std::make_unique<GpsL1Provider>();
        case ConstellationType::GALILEO:
            return std::make_unique<GalileoE1Provider>();
        case ConstellationType::BEIDOU:
            return std::make_unique<BeidouB1Provider>();
        case ConstellationType::GLONASS:
            // TODO: Implement GLONASS FDMA provider separately
            throw QuadGNSSException("GLONASS implementation pending - use FDMA approach");
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

} // namespace QuadGNSS