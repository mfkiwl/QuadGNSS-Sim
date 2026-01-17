# QuadGNSS-Sim - Multi-GNSS Broad-Spectrum Signal Generator

## 🛰️ Project Overview
QuadGNSS-Sim is a production-grade C++ implementation for generating multi-constellation GNSS signals with advanced signal processing capabilities. This system enables **authorized GNSS receiver stress testing** and **vulnerability assessment** through sophisticated broad-spectrum signal generation.

## 🎯 Key Achievements
### 🛰️ Complete Multi-GNSS Architecture
- **✅ GPS L1 C/A**: 1023-chip Gold code generation with authentic ICD-GPS-200 tap delays
- **✅ Galileo E1 OS**: 4092-chip tiered codes with BOC(1,1) modulation
- **✅ BeiDou B1I**: 2046-chip CSS codes with proper frequency planning
- **✅ GLONASS L1**: 14-channel FDMA implementation with per-satellite mixing

### 📊 Broad-Spectrum Generation
- **✅ 60 MSps Sampling**: Covers complete 44.3 MHz GNSS bandwidth
- **✅ 1581.5 MHz Center**: Optimized frequency positioning for all constellations
- **✅ Real-Time Processing**: Optimized for live SDR transmission
- **✅ Multi-Hardware Support**: USRP, BladeRF 2.0, LimeSDR, HackRF compatibility

## 🚀 Quick Start

### Installation
```bash
# Clone repository
git clone [https://github.com/Vanisherzd/QuadGNSS-Sim.git](https://github.com/Vanisherzd/QuadGNSS-Sim.git)
cd QuadGNSS-Sim

# Compile
make all
```
Usage
This tool automatically handles multi-constellation mixing. Please execute the scripts located in the scripts/ directory:

1. Full Broad-Spectrum Mode
Requires 60MSps SDR like USRP, BladeRF, or LimeSDR.
```bash
cd scripts
./run_simulation.sh <sdr_type> <lat> <lon> [hgt]
```
# Example:
# ./run_simulation.sh usrp 40.714 -74.006 100
2. HackRF Mode
Single constellation only due to 20MHz bandwidth hardware limitation.

```bash
cd scripts
./run_single_constellation.sh gps hackrf 40.714 -74.006 100
```

## 🏗️ Documentation
All technical documentation is located in the docs/ directory:

System Architecture

Deployment Guide

Frequency Analysis

Hardware Guide

## 📁 Project Structure
src/: Core C++ source code (Signal generators, Mixer).

include/: Header files and Interface definitions.

scripts/: Deployment and run scripts (Bash/Python).

docs/: Technical documentation and archives.

data/: Ephemeris and satellite data storage.

## 🌟 Acknowledgments
This project builds upon decades of GNSS research:

GPS SDR-SIM - GPS signal algorithms

BDS-SDRSIM - BeiDou B1I signal algorithms

## 📄 License
MIT License - see LICENSE file for details.
