#ifndef QUAD_GNSS_INTERFACE_H
#define QUAD_GNSS_INTERFACE_H

#include <vector>
#include <complex>
#include <memory>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <map>
#include <algorithm>

namespace QuadGNSS {

// GNSS Constellation Types
enum class ConstellationType {
    GPS = 0,
    GLONASS = 1,
    GALILEO = 2,
    BEIDOU = 3,
    NONE = 255
};

// Global Configuration Struct
struct GlobalConfig {
    // Sampling and Frequency Configuration
    static constexpr double DEFAULT_SAMPLING_RATE = 60e6;      // 60 MSps
    static constexpr double DEFAULT_CENTER_FREQ = 1582e6;      // 1582 MHz
    static constexpr double MIN_FREQUENCY = 1561.098e6;       // 1561.098 MHz (Beidou B1)
    static constexpr double MAX_FREQUENCY = 1602.0e6;          // 1602.0 MHz + k×0.5625 MHz (GLONASS L1)
    
    double sampling_rate_hz;
    double center_frequency_hz;
    
    // Active Constellations
    std::vector<ConstellationType> active_constellations;
    
    // Output Configuration
    struct {
        int bits_per_sample = 16;
        double tx_gain_db = 0.0;
        bool enable_iq_file = false;
        std::string output_filename;
    } output;
    
    // Simulation Parameters
    struct {
        double start_time_gps = 0.0;
        double duration_seconds = 60.0;
        bool enable_ionospheric = true;
        bool enable_tropospheric = true;
        bool coherent_mode = false;
    } simulation;
    
    // Constructor with defaults
    GlobalConfig() 
        : sampling_rate_hz(DEFAULT_SAMPLING_RATE)
        , center_frequency_hz(DEFAULT_CENTER_FREQ) {
        // Initialize with all constellations active by default
        active_constellations = {
            ConstellationType::GPS,
            ConstellationType::GLONASS, 
            ConstellationType::GALILEO,
            ConstellationType::BEIDOU
        };
    }
};

// Ephemeris Data Structure
struct EphemerisData {
    int prn;                                    // Satellite PRN
    ConstellationType constellation;           // GNSS constellation
    
    // Keplerian orbital parameters
    double sqrt_a;                               // Square root of semi-major axis (sqrt(m))
    double e;                                     // Eccentricity
    double i0;                                    // Inclination at reference time (rad)
    double omega0;                                 // Right ascension of ascending node at reference time (rad)
    double omega;                                  // Argument of perigee (rad)
    double m0;                                    // Mean anomaly at reference time (rad)
    double delta_n;                                // Mean motion difference from computed value (rad/s)
    double omega_dot;                              // Rate of right ascension change (rad/s)
    double idot;                                   // Rate of inclination change (rad/s)
    double cuc, cus;                              // Harmonic correction terms
    double crc, crs;                              // Harmonic correction terms
    double cic, cis;                              // Harmonic correction terms
    
    // Clock correction parameters
    double clock_bias;                             // Satellite clock bias (s)
    double clock_drift;                            // Satellite clock drift (s/s)
    double clock_drift_rate;                       // Satellite clock drift rate (s/s²)
    
    // Timing parameters
    double toe;                                    // Time of ephemeris (GPS seconds of week)
    double toc;                                    // Time of clock (GPS seconds of week)
    double iodc;                                   // Issue of data, clock
    double iode;                                   // Issue of data, ephemeris
    
    // Validity
    double week_number;                             // GPS week number
    bool is_valid;                                // Data validity flag
    
    EphemerisData() 
        : prn(-1), constellation(ConstellationType::NONE)
        , sqrt_a(0.0), e(0.0), i0(0.0), omega0(0.0), omega(0.0), m0(0.0)
        , delta_n(0.0), omega_dot(0.0), idot(0.0)
        , cuc(0.0), cus(0.0), crc(0.0), crs(0.0), cic(0.0), cis(0.0)
        , clock_bias(0.0), clock_drift(0.0), clock_drift_rate(0.0)
        , toe(0.0), toc(0.0), iodc(0.0), iode(0.0)
        , week_number(0.0), is_valid(false) {}
};

// Satellite Information Structure
struct SatelliteInfo {
    int prn;                                    // Pseudo-Random Number (satellite ID)
    ConstellationType constellation;           // GNSS constellation
    double frequency_hz;                        // Signal frequency (Hz)
    double power_dbm;                          // Signal power (dBm)
    double doppler_hz;                         // Doppler shift (Hz)
    bool is_active;                            // Satellite is active in simulation
    EphemerisData ephemeris;                   // Loaded ephemeris data
    
    SatelliteInfo() 
        : prn(-1), constellation(ConstellationType::NONE)
        , frequency_hz(0.0), power_dbm(-100.0)
        , doppler_hz(0.0), is_active(false) {}
    
    SatelliteInfo(int p, ConstellationType c, double freq)
        : prn(p), constellation(c), frequency_hz(freq)
        , power_dbm(-130.0), doppler_hz(0.0), is_active(true) {}
};

// Custom exception class for QuadGNSS errors
class QuadGNSSException : public std::runtime_error {
public:
    explicit QuadGNSSException(const std::string& message) 
        : std::runtime_error("QuadGNSS Error: " + message) {}
};

// Pure virtual base class for satellite constellations
class ISatelliteConstellation {
public:
    virtual ~ISatelliteConstellation() = default;
    
    /**
     * Generate a chunk of IQ samples for this constellation
     * @param buffer Output buffer for IQ samples (complex<int16_t>)
     * @param sample_count Number of samples to generate
     * @param time_now Current GPS time in seconds
     */
    virtual void generate_chunk(std::complex<int16_t>* buffer, 
                                int sample_count, 
                                double time_now) = 0;
    
    /**
     * Load ephemeris data from file
     * @param file_path Path to ephemeris file (RINEX format)
     * @throws QuadGNSSException if loading fails
     */
    virtual void load_ephemeris(const std::string& file_path) = 0;
    
    /**
     * Set frequency offset for frequency multiplexing
     * @param offset_hz Frequency offset from center frequency in Hz
     */
    virtual void set_frequency_offset(double offset_hz) = 0;
    
    /**
     * Get constellation type
     * @return ConstellationType enum value
     */
    virtual ConstellationType get_constellation_type() const = 0;
    
    /**
     * Get carrier frequency for this constellation
     * @return Carrier frequency in Hz
     */
    virtual double get_carrier_frequency() const = 0;
    
    /**
     * Get list of active satellites in this constellation
     * @return Vector of SatelliteInfo structures
     */
    virtual std::vector<SatelliteInfo> get_active_satellites() const = 0;
    
    /**
     * Configure constellation parameters
     * @param config Global configuration reference
     */
    virtual void configure(const GlobalConfig& config) = 0;
    
    /**
     * Check if constellation is properly initialized and ready
     * @return true if ready for signal generation
     */
    virtual bool is_ready() const = 0;
};

// Main orchestrator class for managing multiple constellations
class SignalOrchestrator {
public:
    /**
     * Constructor
     * @param config Global configuration
     */
    explicit SignalOrchestrator(const GlobalConfig& config);
    
    /**
     * Destructor
     */
    ~SignalOrchestrator();
    
    /**
     * Add a constellation to the orchestrator
     * @param constellation Smart pointer to constellation implementation
     */
    void add_constellation(std::unique_ptr<ISatelliteConstellation> constellation);
    
    /**
     * Initialize all constellations (load ephemeris, configure frequencies)
     * @param ephemeris_file_paths Map of constellation to ephemeris file paths
     */
    void initialize(const std::map<ConstellationType, std::string>& ephemeris_file_paths);
    
    /**
     * Generate mixed IQ signal from all active constellations
     * @param buffer Output buffer for mixed IQ samples
     * @param sample_count Number of samples to generate
     * @param time_now Current GPS time in seconds
     * @throws QuadGNSSException if generation fails
     */
    void mix_all_signals(std::complex<int16_t>* buffer, 
                         int sample_count, 
                         double time_now);
    
    /**
     * Get number of active constellations
     * @return Number of constellations
     */
    size_t get_constellation_count() const;
    
    /**
     * Get list of all active satellites across all constellations
     * @return Vector of SatelliteInfo structures
     */
    std::vector<SatelliteInfo> get_all_satellites() const;
    
    /**
     * Check if orchestrator is ready for signal generation
     * @return true if all constellations are ready
     */
    bool is_ready() const;
    
    /**
     * Get current configuration
     * @return Reference to configuration
     */
    const GlobalConfig& get_config() const;

private:
    // Private member variables
    std::vector<std::unique_ptr<ISatelliteConstellation>> constellations_;
    GlobalConfig config_;
    bool initialized_;
    
    // Private helper methods
    void calculate_frequency_offsets();
    bool validate_configuration() const;
    void prevent_overflow(std::complex<int32_t>* accumulator, int sample_count);
};

// Factory class for creating constellation instances
class ConstellationFactory {
public:
    /**
     * Create a constellation instance based on type
     * @param type Constellation type to create
     * @return Unique pointer to constellation instance
     * @throws QuadGNSSException if type is invalid
     */
    static std::unique_ptr<ISatelliteConstellation> create_constellation(ConstellationType type);
    
    /**
     * Get carrier frequency for a constellation type
     * @param type Constellation type
     * @return Carrier frequency in Hz
     * @throws QuadGNSSException if type is invalid
     */
    static double get_constellation_frequency(ConstellationType type);
    
    /**
     * Get constellation name as string
     * @param type Constellation type
     * @return String representation
     */
    static std::string get_constellation_name(ConstellationType type);
};

} // namespace QuadGNSS

#endif // QUAD_GNSS_INTERFACE_H