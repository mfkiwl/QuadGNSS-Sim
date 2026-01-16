#!/bin/bash
#
# Single Constellation Mode for Hardware-Limited SDRs
# For use with HackRF One (20 MSps max) or other bandwidth-limited devices
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

usage() {
    echo -e "${YELLOW}Usage: $0 <constellation> <sdr_type> <lat> <lon> [hgt]${NC}"
    echo -e "${YELLOW}Constellations: gps, galileo, beidou, glonass${NC}"
    echo -e "${YELLOW}SDR Types: hackrf, limewsdr, usrp, bladerf${NC}"
    echo -e "${YELLOW}Example: $0 gps hackrf 40.714 -74.006 100${NC}"
    exit 1
}

validate_single_constellation() {
    local constellation="$1"
    
    echo -e "${BLUE}Single Constellation Mode:${NC}"
    echo -e "  Selected: ${GREEN}$constellation${NC}"
    echo -e "  Bandwidth: ~2.046 MHz (fits 20 MSps hardware)"
    echo -e "  Center freq: GPS 1575.42 | Galileo 1575.42 | BeiDou 1561.098 | GLONASS 1602.0 MHz"
    echo ""
    
    # Set parameters based on constellation
    case "$constellation" in
        "gps")
            export FREQ_MHZ=1575.42
            export SAMPLE_RATE=20e6
            export BANDWIDTH_MHZ=2.046
            ;;
        "galileo")
            export FREQ_MHZ=1575.42
            export SAMPLE_RATE=20e6
            export BANDWIDTH_MHZ=2.046
            ;;
        "beidou")
            export FREQ_MHZ=1561.098
            export SAMPLE_RATE=20e6
            export BANDWIDTH_MHZ=2.046
            ;;
        "glonass")
            export FREQ_MHZ=1602.0
            export SAMPLE_RATE=20e6
            export BANDWIDTH_MHZ=2.046
            ;;
        *)
            echo -e "${RED}Error: Unknown constellation: $constellation${NC}"
            usage
            ;;
    esac
}

generate_single_sdr_command() {
    local constellation="$1"
    local sdr_type="$2"
    
    echo -e "${BLUE}Single Constellation Transmission:${NC}"
    
    case "$sdr_type" in
        "hackrf")
            cat << EOF
# HackRF Single Constellation Pipeline
./quadgnss_sdr $SAMPLE_RATE ${FREQ_MHZ}e6 "${GPS_EPHEMERIS}" "${GALILEO_EPHEMERIS}" "${BEIDOU_EPHEMERIS}" "${GLONASS_EPHEMERIS}" 2>/dev/null | \
mbuffer -m 100M | \
hackrf_transfer -t 1 -s $SAMPLE_RATE -f ${FREQ_MHZ}e6 -a 1 -x 20 -l 32 -g 20

# Alternative: hackrf_transfer with different parameters
# hackrf_transfer -t 1 -s 20e6 -f 1575.42e6 -a 1 -x 20 -l 32 -g 20
EOF
            ;;
        "limewsdr")
            cat << EOF
# LimeSDR Single Constellation Pipeline  
./quadgnss_sdr $SAMPLE_RATE ${FREQ_MHZ}e6 "${GPS_EPHEMERIS}" "${GALILEO_EPHEMERIS}" "${BEIDOU_EPHEMERIS}" "${GLONASS_EPHEMERIS}" 2>/dev/null | \
mbuffer -m 100M | \
LimeUtil --tx --freq ${FREQ_MHZ}e6 --rate $SAMPLE_RATE --bandwidth 4e6 --gain 20 --antenna LNAW
EOF
            ;;
        "usrp")
            cat << EOF
# USRP Single Constellation Pipeline (overkill but works)
./quadgnss_sdr $SAMPLE_RATE ${FREQ_MHZ}e6 "${GPS_EPHEMERIS}" "${GALILEO_EPHEMERIS}" "${BEIDOU_EPHEMERIS}" "${GLONASS_EPHEMERIS}" 2>/dev/null | \
mbuffer -m 100M | \
uhd_siggen -a "TX_BAND_1" -A "TX1" -s $SAMPLE_RATE -t 30 \
    --wire-format sc16 --args "master_clock_rate=$SAMPLE_RATE" \
    -f ${FREQ_MHZ}e6 -g 20 --stop-on-overflow
EOF
            ;;
        "bladerf")
            cat << EOF
# BladeRF Single Constellation Pipeline
./quadgnss_sdr $SAMPLE_RATE ${FREQ_MHZ}e6 "${GPS_EPHEMERIS}" "${GALILEO_EPHEMERIS}" "${BEIDOU_EPHEMERIS}" "${GLONASS_EPHEMERIS}" 2>/dev/null | \
mbuffer -m 100M | \
bladerf-cli -t tx -s $SAMPLE_RATE -f ${FREQ_MHZ}e6 -g 15 -b 16 -l 5
EOF
            ;;
    esac
    echo ""
}

# Main execution
main() {
    if [ $# -lt 4 ]; then
        usage
    fi
    
    local constellation="$1"
    local sdr_type="$2"
    local lat="$3"
    local lon="$4"
    local hgt="${5:-100}"
    
    validate_single_constellation "$constellation"
    
    # Find ephemeris for specific constellation
    echo -e "${BLUE}Finding Ephemeris...${NC}"
    case "$constellation" in
        "gps")
            gps_file=$(find ./data -name "brdc*.n" -o -name "*.nav" 2>/dev/null | head -1)
            if [ -n "$gps_file" ]; then
                export GPS_EPHEMERIS="$gps_file"
                echo -e "  ${GREEN}✓${NC} GPS: $gps_file"
            else
                echo -e "  ${YELLOW}⚠${NC} No GPS ephemeris found"
                export GPS_EPHEMERIS=""
            fi
            ;;
        "galileo")
            galileo_file=$(find ./data -name "*galileo*.rnx" 2>/dev/null | head -1)
            if [ -n "$galileo_file" ]; then
                export GALILEO_EPHEMERIS="$galileo_file"
                echo -e "  ${GREEN}✓${NC} Galileo: $galileo_file"
            else
                echo -e "  ${YELLOW}⚠${NC} No Galileo ephemeris found"
                export GALILEO_EPHEMERIS=""
            fi
            ;;
        "beidou")
            beidou_file=$(find ./data -name "*beidou*.rnx" 2>/dev/null | head -1)
            if [ -n "$beidou_file" ]; then
                export BEIDOU_EPHEMERIS="$beidou_file"
                echo -e "  ${GREEN}✓${NC} BeiDou: $beidou_file"
            else
                echo -e "  ${YELLOW}⚠${NC} No BeiDou ephemeris found"
                export BEIDOU_EPHEMERIS=""
            fi
            ;;
        "glonass")
            glonass_file=$(find ./data -name "*glonass*.rnx" 2>/dev/null | head -1)
            if [ -n "$glonass_file" ]; then
                export GLONASS_EPHEMERIS="$glonass_file"
                echo -e "  ${GREEN}✓${NC} GLONASS: $glonass_file"
            else
                echo -e "  ${YELLOW}⚠${NC} No GLONASS ephemeris found"
                export GLONASS_EPHEMERIS=""
            fi
            ;;
    esac
    echo ""
    
    generate_single_sdr_command "$constellation" "$sdr_type"
    
    echo -e "${GREEN}Single constellation transmission ready!${NC}"
    echo -e "${BLUE}Coordinates: $lat, $lon, $hgt${NC}"
    echo -e "${BLUE}Sample Rate: $SAMPLE_RATE Hz${NC}"
    echo -e "${BLUE}Center Freq: ${FREQ_MHZ} MHz${NC}"
}

main "$@"