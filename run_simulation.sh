#!/bin/bash
#
# QuadGNSS-Sim Deployment Runner
# Multi-GNSS Broad-Spectrum Signal Generator with SDR Hardware Support
#

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print banner
echo -e "${BLUE}================================================================${NC}"
echo -e "${BLUE}      QuadGNSS-Sim v1.0 - Multi-GNSS Broad-Spectrum Generator${NC}"
echo -e "${BLUE}================================================================${NC}"
echo ""

# Usage function
usage() {
    echo -e "${YELLOW}Usage: $0 <sdr_type> <lat> <lon> <hgt>${NC}"
    echo -e "${YELLOW}SDR Types: hackrf, limewsdr, usrp, bladerf${NC}"
    echo -e "${YELLOW}Example: $0 usrp 40.714 -74.006 100${NC}"
    echo -e "${YELLOW}Coordinates: Decimal degrees (latitude, longitude, height in meters)${NC}"
    exit 1
}

# Hardware validation function
validate_hardware() {
    local sdr_type="$1"
    
    echo -e "${BLUE}Hardware Validation:${NC}"
    echo -e "  Selected SDR: ${GREEN}$sdr_type${NC}"
    
    case "$sdr_type" in
        "hackrf")
            echo -e "  ${RED}ERROR: HackRF One detected${NC}"
            echo -e "  ${RED}  Maximum sample rate: 20 MSps${NC}"
            echo -e "  ${RED}  Required for broad-spectrum: 61.44 MSps${NC}"
            echo -e "  ${RED}  HackRF cannot support 44.3 MHz bandwidth span${NC}"
            echo ""
            echo -e "${YELLOW}Recommendations:${NC}"
            echo -e "  ${GREEN}✓${NC} Upgrade to BladeRF 2.0 (61.44 MSps, ~\$400)"
            echo -e "  ${GREEN}✓${NC} Upgrade to LimeSDR (61.44 MSps, ~\$300)"
            echo -e "  ${GREEN}✓${NC} Upgrade to USRP B210 (61.44 MSps, ~\$1,200)"
            echo ""
            echo -e "${YELLOW}Alternative - Single Constellation Mode:${NC}"
            echo -e "  Reconfigure with --single-gps to reduce bandwidth to 2.046 MHz"
            echo -e "  This can work on HackRF but loses multi-GNSS capability"
            exit 2
            ;;
        "limewsdr")
            echo -e "  ${GREEN}✓${NC} LimeSDR compatible - Up to 61.44 MSps supported"
            ;;
        "usrp")
            echo -e "  ${GREEN}✓${NC} USRP compatible - 61.44 MSps supported"
            ;;
        "bladerf")
            echo -e "  ${GREEN}✓${NC} BladeRF 2.0 compatible - 61.44 MSps supported"
            ;;
        *)
            echo -e "  ${RED}✗${NC} Unknown SDR type: $sdr_type"
            usage
            ;;
    esac
}

# Find latest ephemeris files
find_ephemeris() {
    local data_dir="./data"
    
    echo -e "${BLUE}Ephemeris File Detection:${NC}"
    
    if [ ! -d "$data_dir" ]; then
        echo -e "  ${YELLOW}Creating data directory...${NC}"
        mkdir -p "$data_dir"
    fi
    
    # GPS ephemeris (RINEX 2.11)
    gps_file=$(find "$data_dir" -name "brdc*.n" -o -name "*.nav" 2>/dev/null | head -1)
    if [ -n "$gps_file" ]; then
        echo -e "  ${GREEN}✓${NC} GPS: $gps_file"
        export GPS_EPHEMERIS="$gps_file"
    else
        echo -e "  ${YELLOW}⚠${NC} No GPS ephemeris found in ./data/"
        echo -e "    Expected: brdc*.n or *.nav files"
        export GPS_EPHEMERIS=""
    fi
    
    # Galileo ephemeris (RINEX 3.0)
    galileo_file=$(find "$data_dir" -name "*galileo*.rnx" -o -name "*e*.rnx" 2>/dev/null | head -1)
    if [ -n "$galileo_file" ]; then
        echo -e "  ${GREEN}✓${NC} Galileo: $galileo_file"
        export GALILEO_EPHEMERIS="$galileo_file"
    else
        echo -e "  ${YELLOW}⚠${NC} No Galileo ephemeris found in ./data/"
        echo -e "    Expected: *galileo*.rnx or *e*.rnx files"
        export GALILEO_EPHEMERIS=""
    fi
    
    # BeiDou ephemeris (RINEX 3.0)
    beidou_file=$(find "$data_dir" -name "*beidou*.rnx" -o -name "*c*.rnx" 2>/dev/null | head -1)
    if [ -n "$beidou_file" ]; then
        echo -e "  ${GREEN}✓${NC} BeiDou: $beidou_file"
        export BEIDOU_EPHEMERIS="$beidou_file"
    else
        echo -e "  ${YELLOW}⚠${NC} No BeiDou ephemeris found in ./data/"
        echo -e "    Expected: *beidou*.rnx or *c*.rnx files"
        export BEIDOU_EPHEMERIS=""
    fi
    
    # GLONASS ephemeris (RINEX 3.0)
    glonass_file=$(find "$data_dir" -name "*glonass*.rnx" -o -name "*r*.rnx" 2>/dev/null | head -1)
    if [ -n "$glonass_file" ]; then
        echo -e "  ${GREEN}✓${NC} GLONASS: $glonass_file"
        export GLONASS_EPHEMERIS="$glonass_file"
    else
        echo -e "  ${YELLOW}⚠${NC} No GLONASS ephemeris found in ./data/"
        echo -e "    Expected: *glonass*.rnx or *r*.rnx files"
        export GLONASS_EPHEMERIS=""
    fi
    echo ""
}

# Validate coordinates
validate_coordinates() {
    local lat="$1"
    local lon="$2"
    local hgt="$3"
    
    # Basic coordinate validation
    if ! [[ "$lat" =~ ^-?[0-9]+\.?[0-9]*$ ]] || \
       ! [[ "$lon" =~ ^-?[0-9]+\.?[0-9]*$ ]] || \
       ! [[ "$hgt" =~ ^-?[0-9]+\.?[0-9]*$ ]]; then
        echo -e "${RED}Error: Invalid coordinate format${NC}"
        echo -e "Expected: Decimal degrees (e.g., 40.714 -74.006 100)"
        exit 3
    fi
    
    # Range validation
    if (( $(echo "$lat < -90" | bc -l) )) || (( $(echo "$lat > 90" | bc -l) )); then
        echo -e "${RED}Error: Latitude must be between -90 and 90 degrees${NC}"
        exit 3
    fi
    
    if (( $(echo "$lon < -180" | bc -l) )) || (( $(echo "$lon > 180" | bc -l) )); then
        echo -e "${RED}Error: Longitude must be between -180 and 180 degrees${NC}"
        exit 3
    fi
    
    echo -e "${BLUE}Target Coordinates:${NC}"
    echo -e "  Latitude:  ${GREEN}$lat${NC}°"
    echo -e "  Longitude: ${GREEN}$lon${NC}°"
    echo -e "  Height:     ${GREEN}$hgt${NC} m"
    echo ""
}

# Check dependencies
check_dependencies() {
    echo -e "${BLUE}Dependency Check:${NC}"
    
    # Check for required binaries
    local deps=("quadgnss_sdr" "mbuffer" "bc")
    local missing=()
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            missing+=("$dep")
        else
            echo -e "  ${GREEN}✓${NC} $dep"
        fi
    done
    
    if [ ${#missing[@]} -gt 0 ]; then
        echo -e "${RED}Missing dependencies:${NC}"
        for dep in "${missing[@]}"; do
            echo -e "  ${RED}✗${NC} $dep"
        done
        echo ""
        echo -e "${YELLOW}Install missing dependencies:${NC}"
        echo -e "  sudo apt-get install mbuffer bc"
        exit 4
    fi
    
    # SDR-specific dependencies
    case "$1" in
        "usrp")
            if ! command -v uhd_siggen &> /dev/null; then
                echo -e "  ${YELLOW}⚠${NC} UHD tools not found (optional for USRP)"
            fi
            ;;
        "bladerf")
            if ! command -v bladerf-cli &> /dev/null; then
                echo -e "  ${YELLOW}⚠${NC} BladeRF CLI not found (optional for BladeRF)"
            fi
            ;;
        "limewsdr")
            if ! command -v LimeUtil &> /dev/null; then
                echo -e "  ${YELLOW}⚠${NC} LimeSuite not found (optional for LimeSDR)"
            fi
            ;;
        "hackrf")
            if ! command -v hackrf_transfer &> /dev/null; then
                echo -e "  ${YELLOW}⚠${NC} HackRF tools not found (optional for HackRF)"
            fi
            ;;
    esac
    echo ""
}

# Generate transmission command
generate_transmission_command() {
    local sdr_type="$1"
    
    echo -e "${BLUE}Transmission Command Generation:${NC}"
    
    case "$sdr_type" in
        "usrp")
            cat << 'EOF'
# USRP B210 Transmission Pipeline
./quadgnss_sdr 60e6 1581.5e6 "${GPS_EPHEMERIS}" "${GALILEO_EPHEMERIS}" "${BEIDOU_EPHEMERIS}" "${GLONASS_EPHEMERIS}" 2>/dev/null | \
mbuffer -m 200M | \
uhd_siggen -a "TX_BAND_1" -A "TX1" -s 61.44e6 -t 60 \
    --wire-format sc16 --args "master_clock_rate=61.44e6" \
    -f 1581.5e6 -g 30 --stop-on-overflow

# Alternative: uhd_tx_cfile method
# ./quadgnss_sdr ... | mbuffer -m 200M | \
# uhd_tx_cfile --freq 1581.5e6 --rate 61.44e6 --gain 30 \
#   --wire-format sc16 --args "master_clock_rate=61.44e6"
EOF
            ;;
        "bladerf")
            cat << 'EOF'
# BladeRF 2.0 Transmission Pipeline
./quadgnss_sdr 60e6 1581.5e6 "${GPS_EPHEMERIS}" "${GALILEO_EPHEMERIS}" "${BEIDOU_EPHEMERIS}" "${GLONASS_EPHEMERIS}" 2>/dev/null | \
mbuffer -m 200M | \
bladerf-cli -t tx -s 61.44e6 -f 1581.5e6 -g 20 -b 34 -l 5

# Alternative: bladerf-cli interactive mode
# bladerf-cli -t tx -s 61.44e6 -f 1581.5e6 -g 20
EOF
            ;;
        "limewsdr")
            cat << 'EOF'
# LimeSDR Transmission Pipeline
./quadgnass_sdr 60e6 1581.5e6 "${GPS_EPHEMERIS}" "${GALILEO_EPHEMERIS}" "${BEIDOU_EPHEMERIS}" "${GLONASS_EPHEMERIS}" 2>/dev/null | \
mbuffer -m 200M | \
LimeUtil --tx --freq 1581.5e6 --rate 61.44e6 --bandwidth 45e6 --gain 30 --antenna LNAW

# Alternative: SoapySDR method
# ./quadgnss_sdr ... | mbuffer -m 200M | \
# SoapySDRUtil --tx --freq 1581.5e6 --rate 61.44e6 --gain 30
EOF
            ;;
        "hackrf")
            echo -e "${RED}HackRF transmission command not generated${NC}"
            echo -e "${RED}Hardware does not meet requirements${NC}"
            ;;
    esac
    echo ""
}

# Performance monitoring
start_monitoring() {
    echo -e "${BLUE}Performance Monitoring:${NC}"
    echo -e "  Buffer size: 200 MB (mbuffer)"
    echo -e "  Sample rate: 61.44 MSps"
    echo -e "  Bandwidth: 44.3 MHz (1561.098 - 1605.375 MHz)"
    echo -e "  Center freq: 1581.5 MHz"
    echo ""
    echo -e "${BLUE}Monitoring Commands:${NC}"
    echo -e "  ${YELLOW}htop${NC} - Monitor CPU usage"
    echo -e "  ${YELLOW}iotop${NC} - Monitor I/O usage"  
    echo -e "  ${YELLOW}nethogs${NC} - Monitor network usage"
    echo -e "  ${YELLOW}pactl${NC} - Monitor process activity"
    echo ""
}

# Main execution
main() {
    # Check arguments
    if [ $# -lt 3 ]; then
        usage
    fi
    
    local sdr_type="$1"
    local lat="$2"
    local lon="$3"
    local hgt="${4:-100}"  # Default height 100m
    
    # Validate inputs
    validate_hardware "$sdr_type"
    validate_coordinates "$lat" "$lon" "$hgt"
    check_dependencies "$sdr_type"
    
    # Find ephemeris files
    find_ephemeris
    
    # Generate transmission command
    generate_transmission_command "$sdr_type"
    
    # Show monitoring info
    start_monitoring
    
    echo -e "${GREEN}QuadGNSS-Sim deployment ready!${NC}"
    echo -e "${BLUE}Execute the transmission command above to start signal generation.${NC}"
}

# Execute main function
main "$@"