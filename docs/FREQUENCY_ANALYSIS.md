# QuadGNSS-Sim Frequency Analysis & Transmission Guide

## ? Frequency Offsets & Bandwidth Analysis

### Center Frequency Selection
- **Selected**: 1581.5 MHz
- **Rationale**: Equidistant between lowest BDS (1561.098 MHz) and highest GLONASS (1605.375 MHz)
- **Calculation**: (1561.098 + 1605.375) / 2 = 1581.2365 MHz ??1581.5 MHz

### Exact Frequency Offsets

| Constellation | Carrier Frequency | Offset from Center | Offset Range |
|---|---|---|---|
| **BeiDou B1** | 1561.098 MHz | **-20.402 MHz** | Minimum |
| **GPS L1** | 1575.420 MHz | **-6.080 MHz** | Low |
| **Galileo E1** | 1575.420 MHz | **-6.080 MHz** | Low |
| **GLONASS L1 (k=-7)** | 1598.0625 MHz | **+16.5625 MHz** | Mid |
| **GLONASS L1 (k=0)** | 1602.0000 MHz | **+20.5000 MHz** | Center |
| **GLONASS L1 (k=+6)** | 1605.3750 MHz | **+23.8750 MHz** | Maximum |

### Bandwidth Requirements

#### Total Signal Span
- **Lowest Frequency**: 1561.098 MHz (BeiDou B1)
- **Highest Frequency**: 1605.375 MHz (GLONASS k=+6)
- **Total Span**: 44.277 MHz
- **Required Sample Rate**: 2 ? 22.1385 MHz = **44.28 MHz** (Nyquist minimum)

#### Recommended Sample Rates
| SDR Hardware | Max Sample Rate | Recommended | Status |
|---|---|---|---|
| **USRP B210** | 61.44 MHz | 61.44 MSps | ??**OPTIMAL** |
| **USRP N210** | 100 MSps | 61.44 MSps | ??**OPTIMAL** |
| **BladeRF 2.0** | 61.44 MSps | 61.44 MSps | ??**OPTIMAL** |
| **LimeSDR Mini** | 61.44 MSps | 61.44 MSps | ??**OPTIMAL** |
| **HackRF One** | 20 MSps | 20 MSps | ??**INADEQUATE** |
| **HackRF One** | 20 MSps | 20 MSps | ??**INADEQUATE** |

### Hardware Compatibility Analysis

#### ??**Recommended SDR Hardware**

**USRP Series (uhd_sink_cfile)**
```bash
# Optimal configuration
./quadgnss_sdr | uhd_sink_cfile --type float --freq 1581.5e6 --rate 61.44e6 --gain 30 --tx-size 1024
```
- **Sample Rate**: Up to 61.44 MSps (covers full 44.3 MHz bandwidth)
- **Advantage**: Professional grade, stable, multiple filters
- **Cost**: Higher ($1,000-$3,000)

**BladeRF 2.0 (bladerf-cli)**
```bash
# Optimal configuration
./quadgnss_sdr | bladerf-cli -t tx -s 61.44e6 -l 2000 -g 20 -f 1581.5e6 -b 16
```
- **Sample Rate**: 61.44 MSps (covers full bandwidth)
- **Advantage**: Good performance, reasonable cost
- **Cost**: Medium ($400-$600)

**LimeSDR (LimeUtil)**
```bash
# Optimal configuration
./quadgnss_sdr | LimeUtil --file --freq 1581.5e6 --rate 61.44e6 --gain 30 --bandwidth 45e6
```
- **Sample Rate**: Up to 61.44 MSps
- **Advantage**: Good performance, open-source support
- **Cost**: Medium ($300-$500)

#### ??**Inadequate Hardware (Will Cause Issues)**

**HackRF One (hackrf_transfer)**
```bash
# This will cause aliasing!
./quadgnss_sdr | hackrf_transfer -f 1581.5e6 -s 20e6 -x 1 -a 1 -g 20 -l 32 -r 1000000
```
- **Sample Rate**: 20 MSps (only covers 10 MHz bandwidth)
- **Problem**: Cannot capture 44.3 MHz span
- **Aliasing**: High-frequency components will fold back, corrupting signal

### Signal Aliasing Analysis

#### **Why 20 MSps is Inadequate**

**Nyquist Theory**:
```
Maximum frequency capture = Sample Rate / 2
With 20 MSps: Max frequency = 10 MHz
Our signal range: 1561.098 - 1605.375 MHz
```

**Aliasing Effects**:
```
GLONASS k=+6: 1605.375 MHz
  - Beyond Nyquist (10 MHz)
  - Aliases to: 10 - (1605.375 - 10) = -1595.375 MHz (invalid)
  - Will fold back: 10 - (1605.375 mod 10) = 10 - 5.375 = 4.625 MHz
  - Completely wrong frequency!

GLONASS k=0: 1602.000 MHz
  - Aliases to: 10 - (1602.0 mod 10) = 10 - 2.0 = 8.0 MHz
  - Appears at wrong frequency in band
```

**Filter Rolloff**:
```
Even if hardware claims 20 MSps = 10 MHz bandwidth:
- Real analog filters have rolloff
- 10 MHz filter will attenuate near-edge frequencies
- GLONASS constellations will be severely attenuated
- GPS/Galileo/BeiDou will be attenuated by filter skirts
```

#### **Correct Approach**
```
Required: 2 ? (1605.375 - 1561.098) = 88.554 MHz minimum
Recommended: 61.44 MSps (industry standard for GNSS)
Optimal: 100 MSps (provides ample headroom for filtering)
```

### Transmission Commands

#### **USRP (Recommended)**
```bash
#!/bin/bash
# USRP transmission with optimal parameters
echo "Starting USRP transmission at 1581.5 MHz..."

./quadgnss_sdr | uhd_sink_cfile \
    --type float \
    --freq 1581.5e6 \
    --rate 61.44e6 \
    --gain 30 \
    --tx-size 1024 \
    --lo-offset 0 \
    --file /dev/null \
    --progress

echo "Transmission complete"
```

#### **BladeRF (Recommended)**
```bash
#!/bin/bash
# BladeRF transmission with optimal parameters
echo "Starting BladeRF transmission at 1581.5 MHz..."

bladerf-cli -t tx \
    -s 61.44e6 \
    -l 2000 \
    -g 20 \
    -f 1581.5e6 \
    -b 16 \
    -r 1000000 \
    -p pipe /path/to/quadgnss_sdr

echo "Transmission complete"
```

#### **HackRF (Not Recommended)**
```bash
#!/bin/bash
# HackRF transmission (WARNING: Will cause aliasing!)
echo "WARNING: HackRF 20 MSps inadequate for 44 MHz bandwidth!"
echo "Signal aliasing and corruption will occur!"

./quadgnss_sdr | hackrf_transfer \
    -f 1581.5e6 \
    -s 20e6 \
    -x 1 \
    -a 1 \
    -g 20 \
    -l 32 \
    -r 1000000

echo "Transmission complete (with signal corruption!)"
```

### Performance Comparison

| Sample Rate | Bandwidth Covered | Signal Quality | Hardware Cost | Recommendation |
|---|---|---|---|---|
| **20 MSps** | 10 MHz | **Severely Corrupted** | Low | ??**DO NOT USE** |
| **40 MSps** | 20 MHz | **Partially Corrupted** | Medium | ?? **MINIMAL** |
| **61.44 MSps** | 30.72 MHz | **Good** | Medium-High | ??**RECOMMENDED** |
| **100 MSps** | 50 MHz | **Excellent** | High | ??**OPTIMAL** |

### Filtering Requirements

#### **Recommended Hardware Filtering**
```
- Center: 1581.5 MHz
- Bandwidth: 45-50 MHz
- Passband Ripple: < 0.5 dB
- Stopband Attenuation: > 60 dB
- Transition Bandwidth: < 5 MHz
```

#### **Sample Rate Impact on Filtering**
```
61.44 MSps:
  - Nyquist: 30.72 MHz
  - Filter margin: 14 MHz (good for rolloff)
  - Provides clean passband for all constellations

20 MSps:
  - Nyquist: 10 MHz
  - No margin for filter rolloff
  - Severe distortion of high-frequency components
```

---

## ?? Summary

### ??**Critical Requirements**
1. **Minimum Sample Rate**: 61.44 MSps (to cover 44.3 MHz span)
2. **Recommended Hardware**: USRP, BladeRF 2.0, LimeSDR
3. **Avoid**: HackRF One (20 MSps) - causes signal aliasing

### ?? **Signal Integrity Warning**
Using hardware with insufficient sample rate will cause:
- **Frequency Aliasing**: High GLONASS frequencies fold into wrong locations
- **Filter Rolloff**: Analog filters cannot pass full bandwidth
- **Signal Corruption**: Multi-constellation mixing will be degraded

### ?? **Optimal Configuration**
- **Sample Rate**: 61.44 MSps or higher
- **Center Frequency**: 1581.5 MHz
- **Hardware**: USRP or equivalent professional SDR
- **Expected Performance**: Clean multi-constellation signal with proper frequency separation

---

**QuadGNSS-Sim requires professional SDR hardware for proper signal generation.**
