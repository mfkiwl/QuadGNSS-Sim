# QuadGNSS-Sim C++ Class Architecture

## Overview

The QuadGNSS-Sim C++ Class Architecture provides a modern, object-oriented interface for simulating multiple GNSS constellations (GPS L1, Galileo E1, Beidou B1, and GLONASS L1) simultaneously for authorized receiver stress testing.

## Architecture Design

### Core Components

#### 1. `ISatelliteConstellation` - Pure Virtual Interface

The foundation of the architecture is a pure virtual base class that defines the contract for all GNSS constellation implementations.

```cpp
class ISatelliteConstellation {
public:
    virtual void generate_chunk(std::complex<int16_t>* buffer, 
                                int sample_count, 
                                double time_now) = 0;
    virtual void load_ephemeris(const std::string& file_path) = 0;
    virtual void set_frequency_offset(double offset_hz) = 0;
    // ... other virtual methods
};
```

**Key Methods:**
- `generate_chunk()`: Core signal generation method
- `load_ephemeris()`: Load navigation data from RINEX files
- `set_frequency_offset()`: Critical for frequency multiplexing

#### 2. `SignalOrchestrator` - Master Controller

Coordinates multiple constellation instances and produces the final mixed IQ stream.

```cpp
class SignalOrchestrator {
private:
    std::vector<std::unique_ptr<ISatelliteConstellation>> constellations_;
    GlobalConfig config_;
    
public:
    void mix_all_signals(std::complex<int16_t>* buffer, 
                         int sample_count, 
                         double time_now);
    // ... other methods
};
```

**Key Features:**
- Manages constellation lifecycle
- Implements overflow protection
- Handles frequency multiplexing
- Provides unified signal output

#### 3. `GlobalConfig` - Configuration Management

Centralized configuration structure with compile-time constants for the target system.

```cpp
struct GlobalConfig {
    static constexpr double DEFAULT_SAMPLING_RATE = 60e6;      // 60 MSps
    static constexpr double DEFAULT_CENTER_FREQ = 1582e6;      // 1582 MHz
    static constexpr double MIN_FREQUENCY = 1561.098e6;       // Beidou B1
    static constexpr double MAX_FREQUENCY = 1602.0e6;          // GLONASS L1
};
```

**Design Rationale:**
- 60 MSps sampling covers 41 MHz bandwidth (1561-1602 MHz)
- Center frequency of 1582 MHz calculated as midpoint
- Allows for proper frequency multiplexing

#### 4. `ConstellationFactory` - Creation Pattern

Factory pattern for creating constellation instances with type safety.

```cpp
class ConstellationFactory {
public:
    static std::unique_ptr<ISatelliteConstellation> create_constellation(ConstellationType type);
    static double get_constellation_frequency(ConstellationType type);
    static std::string get_constellation_name(ConstellationType type);
};
```

## Technical Implementation Details

### Frequency Multiplexing Strategy

The system uses frequency offset positioning to multiplex all constellations:

1. **GPS L1**: 1575.42 MHz (BPSK modulation)
2. **GLONASS L1**: 1602.0 MHz + k×0.5625 MHz (FDMA)
3. **Galileo E1**: 1575.42 MHz (BOC(1,1) modulation)
4. **Beidou B1**: 1561.098 MHz (BPSK modulation)

### Signal Processing Pipeline

1. **Individual Generation**: Each constellation generates its IQ samples
2. **Frequency Offset**: Applied via `set_frequency_offset()` for multiplexing
3. **Accumulation**: Signals combined using 32-bit accumulator
4. **Overflow Protection**: Automatic scaling to prevent int16_t overflow
5. **Final Output**: Converted to 16-bit IQ samples

### Memory Management

- **RAII**: Smart pointers (`std::unique_ptr`) for automatic cleanup
- **No Raw Pointers**: All memory managed through STL containers
- **Exception Safety**: Proper error handling without resource leaks

## Usage Example

### Basic Setup

```cpp
#include "quad_gnss_interface.h"
using namespace QuadGNSS;

// 1. Configure global parameters
GlobalConfig config;
config.sampling_rate_hz = 60e6;  // 60 MSps
config.center_frequency_hz = 1582e6;  // 1582 MHz

// 2. Create orchestrator
SignalOrchestrator orchestrator(config);

// 3. Add constellations
auto gps = ConstellationFactory::create_constellation(ConstellationType::GPS);
auto glonass = ConstellationFactory::create_constellation(ConstellationType::GLONASS);
orchestrator.add_constellation(std::move(gps));
orchestrator.add_constellation(std::move(glonass));

// 4. Initialize with ephemeris data
std::map<ConstellationType, std::string> ephemeris_files = {
    {ConstellationType::GPS, "gps_ephemeris.dat"},
    {ConstellationType::GLONASS, "glonass_ephemeris.dat"}
};
orchestrator.initialize(ephemeris_files);
```

### Signal Generation

```cpp
// Generate IQ samples
const int sample_count = 1000000;  // 1 million samples
std::vector<std::complex<int16_t>> signal_buffer(sample_count);
double current_time = 0.0;  // GPS time in seconds

orchestrator.mix_all_signals(signal_buffer.data(), 
                             sample_count, 
                             current_time);
```

## Class Hierarchy

```
ISatelliteConstellation (pure virtual interface)
├── GPSConstellation      (1575.42 MHz, BPSK)
├── GLONASSConstellation  (1602.0 MHz + k×0.5625 MHz, FDMA)
├── GalileoConstellation  (1575.42 MHz, BOC(1,1))
└── BeidouConstellation   (1561.098 MHz, BPSK)

SignalOrchestrator
├── std::vector<std::unique_ptr<ISatelliteConstellation>> constellations_
├── GlobalConfig config_
└── mix_all_signals() method
```

## Key Benefits

### 1. **Extensibility**
- Easy to add new GNSS constellations
- Plugin-style architecture
- Interface-based design

### 2. **Performance**
- Zero-overhead abstractions
- Efficient memory management
- Optimized signal processing

### 3. **Type Safety**
- Strong typing with enums
- Smart pointer usage
- Exception-based error handling

### 4. **Testability**
- Interface-based design enables easy mocking
- Separation of concerns
- Modular testing possible

## Migration Path

The architecture supports gradual migration from the existing C codebase:

1. **Phase 1**: Create C++ interface alongside C code
2. **Phase 2**: Implement constellation classes using existing C functions
3. **Phase 3**: Migrate main simulation engine
4. **Phase 4**: Optimize and modernize individual components

## File Structure

```
include/
└── quad_gnss_interface.h    # Main interface header

src/
├── core/
│   ├── satellite_constellation.cpp    # Base implementation
│   └── signal_orchestrator.cpp        # Master controller
├── constellations/
│   ├── gps_constellation.cpp          # GPS L1 C/A implementation
│   ├── glonass_constellation.cpp      # GLONASS L1 implementation
│   ├── galileo_constellation.cpp      # Galileo E1 OS implementation
│   └── beidou_constellation.cpp       # BeiDou B1I implementation
└── demonstrations/
    ├── demonstration.cpp              # Usage examples
    └── interface_test.cpp             # Unit tests
```

## Performance Specifications

- **Sampling Rate**: 60 MSps (configurable)
- **Frequency Coverage**: 1561-1602 MHz
- **Signal Formats**: 16-bit complex IQ samples
- **Constellation Count**: Up to 4 simultaneous
- **Satellite Count**: Up to 64 total across all constellations
- **Real-time Processing**: Optimized for live SDR transmission

## Security Considerations

The architecture is designed for authorized receiver testing only:
- Built-in safeguards against malicious use
- Configurable power limits
- Proper frequency band adherence
- Integration with existing security frameworks

## Conclusion

The QuadGNSS-Sim C++ Class Architecture provides a robust, modern foundation for multi-GNSS signal generation with excellent extensibility, performance, and maintainability characteristics. The interface-based design supports easy integration with existing SDR hardware while providing a clean abstraction for future constellation additions.