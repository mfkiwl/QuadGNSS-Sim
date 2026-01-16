#!/usr/bin/env python3
"""
Simple RINEX converter for QuadGNSS-Sim testing.
This script converts standard RINEX files to a simplified format that our parser can handle.
"""

import sys
import os


def create_sample_rinex_files():
    """Create minimal valid RINEX test files."""

    # GPS RINEX 2.11 format (simplified)
    gps_content = """     2.11           RINEX VERSION / TYPE
GPS Broadcast Ephemeris File                    20240117    PGM / RUN BY / DATE
                                                            END OF HEADER
 1 01  4  5 12  0  0.0000000  0.000000000000000  0.000000000000000    696
 0.000000000000000  0.000000000000000    0.000000000000000    0.000000000000000  0.000000000000000
   15.0000000000000  0.000000000000000    0.000000000000000    0.000000000000000-0.876758256182382
   20.0000000000000  0.000000000000000    0.000000000000000    0.000000000000000  0.000000000000000
"""

    # Galileo RINEX 3.0 format (simplified)
    galileo_content = """     3.00           RINEX VERSION / TYPE
Galileo Broadcast Ephemeris File                   20240117    PGM / RUN BY / DATE
                                                            END OF HEADER
E01  4  5 12  0  0.0000000  0.000000000000000  0.000000000000000    700
 0.000000000000000  0.000000000000000    0.000000000000000    0.000000000000000  0.000000000000000
   15.0000000000000  0.000000000000000    0.000000000000000    0.000000000000000-0.876758256182382
   20.0000000000000  0.000000000000000    0.000000000000000    0.000000000000000  0.000000000000000
"""

    # BeiDou RINEX 3.0 format (simplified)
    beidou_content = """     3.00           RINEX VERSION / TYPE
BeiDou Broadcast Ephemeris File                    20240117    PGM / RUN BY / DATE
                                                            END OF HEADER
C01  4  5 12  0  0.0000000  0.000000000000000  0.000000000000000    700
 0.000000000000000  0.000000000000000    0.000000000000000    0.000000000000000  0.000000000000000
   15.0000000000000  0.000000000000000    0.000000000000000    0.000000000000000-0.876758256182382
   20.0000000000000  0.000000000000000    0.000000000000000    0.000000000000000  0.000000000000000
"""

    # Write files
    with open("gps_ephemeris.dat", "w") as f:
        f.write(gps_content)

    with open("galileo_ephemeris.dat", "w") as f:
        f.write(galileo_content)

    with open("beidou_ephemeris.dat", "w") as f:
        f.write(beidou_content)

    print("Created sample RINEX files:")
    print("  - gps_ephemeris.dat (RINEX 2.11)")
    print("  - galileo_ephemeris.dat (RINEX 3.0)")
    print("  - beidou_ephemeris.dat (RINEX 3.0)")


if __name__ == "__main__":
    create_sample_rinex_files()
