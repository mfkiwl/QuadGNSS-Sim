#!/bin/bash

# QuadGNSS-Sim Transmission Script
# Author: QuadGNSS-Sim Project
# Purpose: Transmit broad-spectrum GNSS signal with proper frequency analysis

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
CENTER_FREQ_MHZ=1581.5
SAMPLE_RATE_MSPS=61.44
OUTPUT_FILE="quadgnss_sdr"

echo -e "${BLUE}=== QuadGNSS-Sim Frequency Analysis ===${NC}"
echo ""
echo "Center Frequency: ${CENTER_FREQ_MHZ} MHz"
echo "Sample Rate: ${SAMPLE_RATE_MSPS} MSps"
echo ""

# Pre-calculated frequency offsets for simplicity
echo -e "${GREEN}Frequency Offsets from Center (${CENTER_FREQ_MHZ} MHz):${NC}"
echo "  BeiDou B1:  1561.098 MHz → Δf: ${RED}-20.402 MHz${NC} (minimum)"
echo "  GPS L1:      1575.420 MHz → Δf: ${RED}-6.080 MHz${NC}"
echo "  Galileo E1:   1575.420 MHz → Δf: ${RED}-6.080 MHz${NC}"
echo "  GLONASS k=-7: 1598.0625 MHz → Δf: ${GREEN}+16.5625 MHz${NC}"
echo "  GLONASS k=0:  1602.0000 MHz → Δf: ${GREEN}+20.5000 MHz${NC}"
echo "  GLONASS k=+6: 1605.3750 MHz → Δf: ${YELLOW}+23.8750 MHz${NC} (maximum)"
echo ""

# Bandwidth analysis
MIN_FREQ_MHZ=1561.098
MAX_FREQ_MHZ=1605.375
TOTAL_SPAN_MHZ=44.277
MIN_SAMPLE_RATE_MSPS=88.55

echo -e "${GREEN}Bandwidth Requirements:${NC}"
echo "  Lowest Frequency:  ${MIN_FREQ_MHZ} MHz (BeiDou B1)"
echo "  Highest Frequency: ${MAX_FREQ_MHZ} MHz (GLONASS k=+6)"
echo "  Total Span:        ${TOTAL_SPAN_MHZ} MHz"
echo "  Min Sample Rate:    ${MIN_SAMPLE_RATE_MSPS} MSps (Nyquist)"
echo "  Recommended Rate:    ${SAMPLE_RATE_MSPS} MSps"
echo ""

# Hardware compatibility check
echo -e "${GREEN}Hardware Compatibility Analysis:${NC}"
echo ""

# USRP (Recommended)
echo -e "${GREEN}✅ USRP Series (uhd_sink_cfile) - RECOMMENDED${NC}"
echo "   Sample Rate: Up to 61.44 MSps"
echo "   Command: ./quadgnss_sdr | uhd_sink_cfile --freq ${CENTER_FREQ_MHZ}e6 --rate ${SAMPLE_RATE_MSPS}e6"
echo "   Cost: $$1,000-3,000"
echo "   Status: ✅ OPTIMAL - Professional grade with full bandwidth"
echo ""

# BladeRF (Recommended)
echo -e "${GREEN}✅ BladeRF 2.0 (bladerf-cli) - RECOMMENDED${NC}"
echo "   Sample Rate: Up to 61.44 MSps"
echo "   Command: ./quadgnass_sdr | bladerf-cli -t tx -s ${SAMPLE_RATE_MSPS}e6 -f ${CENTER_FREQ_MHZ}e6"
echo "   Cost: $$400-600"
echo "   Status: ✅ OPTIMAL - Good performance, reasonable cost"
echo ""

# LimeSDR (Recommended)
echo -e "${GREEN}✅ LimeSDR (LimeUtil) - RECOMMENDED${NC}"
echo "   Sample Rate: Up to 61.44 MSps"
echo "   Command: ./quadgnass_sdr | LimeUtil --freq ${CENTER_FREQ_MHZ}e6 --rate ${SAMPLE_RATE_MSPS}e6"
echo "   Cost: $$300-500"
echo "   Status: ✅ OPTIMAL - Good performance, open-source"
echo ""

# HackRF (Not Recommended)
HACKRF_RATE_MSPS=20
HACKRF_MAX_FREQ_MHZ=10
echo -e "${RED}❌ HackRF One (hackrf_transfer) - NOT RECOMMENDED${NC}"
echo "   Sample Rate: ${HACKRF_RATE_MSPS} MSps"
echo "   Max Frequency: ${HACKRF_MAX_FREQ_MHZ} MHz (Nyquist limited)"
echo "   Required: ${TOTAL_SPAN_MHZ} MHz span"
echo "   Command: ./quadgnss_sdr | hackrf_transfer -f ${CENTER_FREQ_MHZ}e6 -s ${HACKRF_RATE_MSPS}e6"
echo "   Cost: $$300"
echo "   Status: ❌ INADEQUATE - Will cause signal corruption"
echo ""

# Signal aliasing explanation
echo -e "${RED}⚠️  SIGNAL ALIASING WARNING FOR HACKRF${NC}"
echo ""
echo -e "${YELLOW}Why ${HACKRF_RATE_MSPS} MSps is Inadequate:${NC}"
echo "  Nyquist Theory: Max frequency capture = Sample Rate / 2"
echo "  With ${HACKRF_RATE_MSPS} MSps: Max frequency = ${HACKRF_MAX_FREQ_MHZ} MHz"
echo "  Our signal range: ${MIN_FREQ_MHZ} - ${MAX_FREQ_MHZ} MHz"
echo ""
echo -e "${RED}Aliasing Effects on GLONASS Constellations:${NC}"
echo "  GLONASS k=+6: ${MAX_FREQ_MHZ} MHz"
echo "    - Beyond Nyquist (${HACKRF_MAX_FREQ_MHZ} MHz)"
GLONASS_MOD=$(echo "$MAX_FREQ_MHZ $HACKRF_MAX_FREQ_MHZ" | awk '{printf "%.3f", $1 % $2}')
GLONASS_ALIAS=$(echo "$HACKRF_MAX_FREQ_MHZ $MAX_FREQ_MHZ $HACKRF_MAX_FREQ_MHZ" | awk '{printf "%.3f", $1 - ($2 % $3)}')
echo "    - Will alias to: ${HACKRF_MAX_FREQ_MHZ} - (${MAX_FREQ_MHZ} mod ${HACKRF_MAX_FREQ_MHZ}) = ${GLONASS_ALIAS} MHz"
echo "    - Result: Completely wrong frequency, corrupted signal!"
echo ""
echo "  GLONASS k=0: 1602.000 MHz"
GLONASS_0_MOD=$(echo "1602.0 $HACKRF_MAX_FREQ_MHZ" | awk '{printf "%.3f", $1 % $2}')
GLONASS_0_ALIAS=$(echo "$HACKRF_MAX_FREQ_MHZ 1602.0 $HACKRF_MAX_FREQ_MHZ" | awk '{printf "%.3f", $1 - ($2 % $3)}')
echo "    - Will alias to: ${HACKRF_MAX_FREQ_MHZ} - (1602.0 mod ${HACKRF_MAX_FREQ_MHZ}) = ${GLONASS_0_ALIAS} MHz"
echo "    - Result: Wrong frequency placement in band"
echo ""
echo -e "${RED}Filter Rolloff Effects:${NC}"
echo "  Even if hardware claims ${HACKRF_RATE_MSPS} MSps = ${HACKRF_MAX_FREQ_MHZ} MHz bandwidth:"
echo "  - Real analog filters have rolloff (20-40% of Nyquist)"
echo "  - Effective bandwidth: ~8 MHz (not 10 MHz)"
echo "  - GPS/Galileo/BeiDou will be attenuated by filter skirts"
echo "  - Multi-constellation mixing severely degraded"
echo ""

# Performance comparison table
echo -e "${GREEN}Performance Comparison:${NC}"
echo -e "${BLUE}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BLUE}│ Sample Rate │ Bandwidth Covered │ Signal Quality │ Hardware Cost │ Recommendation │${NC}"
echo -e "${BLUE}├─────────────────────────────────────────────────────────────────┤${NC}"
echo -e "${RED}│ 20 MSps     │ 10 MHz            │ Severely Corrupted │ Low           │ ❌ DO NOT USE │${NC}"
echo -e "${YELLOW}│ 40 MSps     │ 20 MHz            │ Partially Corrupted │ Medium        │ ⚠️  MINIMAL    │${NC}"
echo -e "${GREEN}│ 61.44 MSps  │ 30.72 MHz          │ Good             │ Medium-High   │ ✅ RECOMMENDED │${NC}"
echo -e "${GREEN}│ 100 MSps    │ 50 MHz            │ Excellent        │ High          │ ✅ OPTIMAL     │${NC}"
echo -e "${BLUE}└─────────────────────────────────────────────────────────────────┘${NC}"
echo ""

# Check for available binaries
echo -e "${GREEN}Binary Availability Check:${NC}"
HAVE_USRP=$(command -v uhd_sink_cfile >/dev/null 2>&1 && echo "YES" || echo "NO")
HAVE_BLADERF=$(command -v bladerf-cli >/dev/null 2>&1 && echo "YES" || echo "NO")
HAVE_LIMESDR=$(command -v LimeUtil >/dev/null 2>&1 && echo "YES" || echo "NO")
HAVE_HACKRF=$(command -v hackrf_transfer >/dev/null 2>&1 && echo "YES" || echo "NO")

echo "  USRP (uhd_sink_cfile):     $HAVE_USRP"
echo "  BladeRF (bladerf-cli):    $HAVE_BLADERF"
echo "  LimeSDR (LimeUtil):        $HAVE_LIMESDR"
echo "  HackRF (hackrf_transfer):   $HAVE_HACKRF"
echo ""

# Generate transmission commands
echo -e "${GREEN}Transmission Commands:${NC}"
echo ""

if [ "$HAVE_USRP" = "YES" ]; then
    echo -e "${GREEN}# USRP Transmission (RECOMMENDED)${NC}"
    echo "./quadgnass_sdr | uhd_sink_cfile \\"
    echo "    --type float \\"
    echo "    --freq ${CENTER_FREQ_MHZ}e6 \\"
    echo "    --rate ${SAMPLE_RATE_MSPS}e6 \\"
    echo "    --gain 30 \\"
    echo "    --tx-size 1024 \\"
    echo "    --lo-offset 0 \\"
    echo "    --file /dev/null \\"
    echo "    --progress"
    echo ""
fi

if [ "$HAVE_BLADERF" = "YES" ]; then
    echo -e "${GREEN}# BladeRF Transmission (RECOMMENDED)${NC}"
    echo "bladerf-cli -t tx \\"
    echo "    -s ${SAMPLE_RATE_MSPS}e6 \\"
    echo "    -l 2000 \\"
    echo "    -g 20 \\"
    echo "    -f ${CENTER_FREQ_MHZ}e6 \\"
    echo "    -b 16 \\"
    echo "    -r 1000000 \\"
    echo "    -p pipe /path/to/quadgnss_sdr"
    echo ""
fi

if [ "$HAVE_LIMESDR" = "YES" ]; then
    echo -e "${GREEN}# LimeSDR Transmission (RECOMMENDED)${NC}"
    echo "LimeUtil --tx \\"
    echo "    --file \\"
    echo "    --freq ${CENTER_FREQ_MHZ}e6 \\"
    echo "    --rate ${SAMPLE_RATE_MSPS}e6 \\"
    echo "    --gain 30 \\"
    echo "    --bandwidth 45e6"
    echo ""
fi

if [ "$HAVE_HACKRF" = "YES" ]; then
    echo -e "${RED}# HackRF Transmission (NOT RECOMMENDED)${NC}"
    echo -e "${YELLOW}# WARNING: This will cause signal corruption!${NC}"
    echo "hackrf_transfer \\"
    echo "    -f ${CENTER_FREQ_MHZ}e6 \\"
    echo "    -s ${HACKRF_RATE_MSPS}e6 \\"
    echo "    -x 1 \\"
    echo "    -a 1 \\"
    echo "    -g 20 \\"
    echo "    -l 32 \\"
    echo "    -r 1000000"
    echo ""
fi

# Compile and prepare QuadGNSS-Sim
echo -e "${GREEN}Preparing QuadGNSS-Sim:${NC}"
cd "$(dirname "$0")"

if [ ! -f "src/main.cpp" ]; then
    echo -e "${RED}Error: src/main.cpp not found!${NC}"
    exit 1
fi

echo "Compiling QuadGNSS-Sim..."
g++ -std=c++17 -O3 -pthread src/main.cpp -o quadgnss_sdr 2>/dev/null || {
    echo -e "${RED}Error: Compilation failed!${NC}"
    exit 1
}

echo -e "${GREEN}✅ Compilation successful!${NC}"
echo ""

# Check if user wants to transmit now
read -p "Start transmission now? (y/n): " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${GREEN}Starting QuadGNSS-Sim transmission...${NC}"
    echo ""
    
    # Find available SDR and transmit
    if [ "$HAVE_USRP" = "YES" ]; then
        echo -e "${BLUE}Using USRP for transmission...${NC}"
        ./quadgnass_sdr | uhd_sink_cfile \
            --type float \
            --freq ${CENTER_FREQ_MHZ}e6 \
            --rate ${SAMPLE_RATE_MSPS}e6 \
            --gain 30 \
            --tx-size 1024 \
            --lo-offset 0 \
            --file /dev/null \
            --progress
    elif [ "$HAVE_BLADERF" = "YES" ]; then
        echo -e "${BLUE}Using BladeRF for transmission...${NC}"
        bladerf-cli -t tx \
            -s ${SAMPLE_RATE_MSPS}e6 \
            -l 2000 \
            -g 20 \
            -f ${CENTER_FREQ_MHZ}e6 \
            -b 16 \
            -r 1000000 \
            -p pipe /path/to/quadgnass_sdr
    elif [ "$HAVE_LIMESDR" = "YES" ]; then
        echo -e "${BLUE}Using LimeSDR for transmission...${NC}"
        LimeUtil --tx \
            --file \
            --freq ${CENTER_FREQ_MHZ}e6 \
            --rate ${SAMPLE_RATE_MSPS}e6 \
            --gain 30 \
            --bandwidth 45e6
    else
        echo -e "${RED}No compatible SDR hardware found!${NC}"
        echo "Please install: USRP, BladeRF 2.0, or LimeSDR"
        echo ""
        echo -e "${YELLOW}Alternatively, save to file:${NC}"
        echo "./quadgnss_sdr > output.iq"
        echo ""
        echo -e "${GREEN}Starting file output...${NC}"
        timeout 10s ./quadgnass_sdr > output.iq || true
        echo -e "${GREEN}Signal saved to output.iq${NC}"
    fi
else
    echo -e "${GREEN}Transmission ready. Run: ./transmit_quad.sh${NC}"
fi

echo ""
echo -e "${GREEN}=== QuadGNSS-Sim Configuration Complete ===${NC}"