# QuadGNSS-Sim Deployment Scripts

## Quick Start

### Multi-Constellation Mode (60 MSps Required)
```bash
# Full broad-spectrum GNSS (44.3 MHz bandwidth)
scripts/run_simulation.sh <sdr_type> <lat> <lon> [hgt]

# Examples:
scripts/run_simulation.sh usrp 40.714 -74.006 100     # USRP B210
scripts/run_simulation.sh bladerf 40.714 -74.006 100   # BladeRF 2.0
scripts/run_simulation.sh limewsdr 40.714 -74.006 100   # LimeSDR
scripts/run_simulation.sh hackrf 40.714 -74.006 100     # WILL FAIL (20 MSps max)
```

### Single Constellation Mode (20 MSps Compatible)
```bash
# For bandwidth-limited hardware (HackRF One, etc.)
scripts/run_single_constellation.sh <constellation> <sdr_type> <lat> <lon> [hgt]

# Examples:
scripts/run_single_constellation.sh gps hackrf 40.714 -74.006 100
scripts/run_single_constellation.sh galileo hackrf 40.714 -74.006 100
scripts/run_single_constellation.sh beidou hackrf 40.714 -74.006 100
```

## Hardware Compatibility

### ??Recommended Hardware (60 MSps+)
| SDR | Max Sample Rate | Cost | Status |
|-----|-----------------|-------|---------|
| USRP B210 | 61.44 MSps | ~$1,200 | ??Optimal |
| BladeRF 2.0 | 61.44 MSps | ~$400 | ??Optimal |
| LimeSDR | 61.44 MSps | ~$300 | ??Optimal |

### ?? Limited Hardware (20 MSps Max)
| SDR | Max Sample Rate | Cost | Status |
|-----|-----------------|-------|---------|
| HackRF One | 20 MSps | ~$300 | ?? Single constellation only |

## Setup Instructions

### 1. Dependencies
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake git bc mbuffer

# SDR-specific packages
sudo apt-get install uhd-host libuhd-dev          # USRP
sudo apt-get install bladerf libbladerf-dev      # BladeRF
sudo apt-get install limesuite liblimesuite-dev   # LimeSDR
sudo apt-get install hackrf libhackrf-dev          # HackRF
```

### 2. Ephemeris Data
```bash
# Create data directory
mkdir -p data

# Download GPS broadcast ephemeris (example)
wget -O data/brdc0540.23n "ftp://igs.ign.ethz.ch/igs/data/2023/053/brdc0540.23n.gz"
gunzip data/brdc0540.23n.gz

# Download multi-constellation RINEX 3.0 files
wget -O data/multi_constellation.rnx "https://igs.org/pub/data/example.rnx"
```

### 3. Compilation
```bash
# Build QuadGNSS-Sim
cd /path/to/quadgnss-sim
make all

# Verify binary
./quadgnss_sdr --help
```

### 4. Run Simulation
```bash
# Full multi-constellation (requires 61.44+ MSps SDR)
scripts/run_simulation.sh usrp 40.714 -74.006 100

# Single constellation (works on 20 MSps SDRs)
scripts/run_single_constellation.sh gps hackrf 40.714 -74.006 100
```

## Signal Specifications

### Multi-Constellation Mode
- **Total Bandwidth**: 44.3 MHz (1561.098 - 1605.375 MHz)
- **Sample Rate**: 61.44 MSps
- **Center Frequency**: 1581.5 MHz
- **Constellations**: GPS + GLONASS + Galileo + BeiDou
- **Modulation**: CDMA (GPS/Galileo/BeiDou) + FDMA (GLONASS)

### Single Constellation Mode
- **Bandwidth**: ~2.046 MHz
- **Sample Rate**: 20 MSps (HackRF max)
- **Center Frequency**: Varies by constellation
- **Single Constellation**: User-selected GPS/Galileo/BeiDou/GLONASS
- **Modulation**: CDMA or FDMA (single system)

## Pipeline Optimization

### FIFO Buffering (mbuffer)
```bash
# Prevents disk I/O bottlenecks and underruns
./quadgnss_sdr ... | mbuffer -m 200M | tx_command

# Buffer sizes:
# -m 100M  (100 MB - good for single constellation)
# -m 200M  (200 MB - good for multi-constellation)
# -m 500M  (500 MB - high-end systems)
```

### Real-time Priority
```bash
# For low-latency applications
sudo nice -n -10 scripts/run_simulation.sh usrp 40.714 -74.006 100

# Or use chrt for real-time scheduling
sudo chrt -f 99 scripts/run_simulation.sh usrp 40.714 -74.006 100
```

## Troubleshooting

### Hardware Not Found
```bash
# Check device permissions
sudo usermod -a -G dialout $USER

# Check device visibility
ls -la /dev/bus/usb/    # USB devices
uhd_find_devices       # USRP devices
bladerf-cli -p        # BladeRF devices
LimeUtil --find       # LimeSDR devices
hackrf_list           # HackRF devices
```

### Buffer Underruns
```bash
# Increase buffer size
scripts/run_simulation.sh usrp 40.714 -74.006 100 2>&1 | grep buffer

# Reduce CPU load
# Lower sample rate for single constellation mode
# Disable unnecessary system services
# Use high-performance CPU governor
sudo cpupower frequency-set -g performance
```

### Signal Quality Issues
```bash
# Check frequency plans
scripts/run_simulation.sh usrp 40.714 -74.006 100 2>&1 | grep -i frequency

# Verify ephemeris freshness
find data -name "*.n*" -mtime -7  # Files newer than 7 days

# Check antenna placement
# Ensure clear sky view
# Minimize multipath reflections
# Use proper antenna polarization
```

## Advanced Usage

### Custom Frequency Plans
```bash
# Override default frequencies
export CENTER_FREQ_MHZ=1575.42    # GPS/Galileo
export SAMPLE_RATE=30e6            # Custom rate
scripts/run_simulation.sh usrp 40.714 -74.006 100
```

### GNSS Constellation Selection
```bash
# Disable specific constellations
export DISABLE_GLONASS=1          # Skip GLONASS
export DISABLE_BEIDOU=1           # Skip BeiDou
scripts/run_simulation.sh usrp 40.714 -74.006 100
```

### Debug Mode
```bash
# Enable verbose output
export QUADGNSS_DEBUG=1
scripts/run_simulation.sh usrp 40.714 -74.006 100

# Log to file
scripts/run_simulation.sh usrp 40.714 -74.006 100 2>&1 | tee simulation.log
```

## Performance Monitoring

### System Resources
```bash
# Monitor CPU usage (during transmission)
htop

# Monitor memory usage
free -h

# Monitor I/O usage
iotop

# Monitor system temperatures
sensors
```

### Signal Analysis
```bash
# Capture signal for analysis
scripts/run_simulation.sh usrp 40.714 -74.006 100 | \
mbuffer -m 200M | \
tee signal_capture.iq | \
uhd_siggen ...

# Analyze captured signal
# GNU Radio Companion
grc signal_capture.iq

# Spectral analysis
hackrf_sweep -f 1560:1610:1 -w 20 -l 20 -1
```

## Emergency Stop
```bash
# Ctrl+C to stop transmission
# Or kill process if needed
pkill -f quadgnss_sdr
pkill -f uhd_siggen
pkill -f hackrf_transfer
```
