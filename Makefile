CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -D_GNU_SOURCE
LDFLAGS = -lm -lpthread -lcurl -lncurses -lz

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = $(SRCDIR)/multi_gnss_sim.c \
          $(SRCDIR)/constellation/gps.c \
          $(SRCDIR)/constellation/glonass.c \
          $(SRCDIR)/constellation/galileo.c \
          $(SRCDIR)/constellation/beidou.c \
          $(SRCDIR)/rinex/rinex_parser.c \
          $(SRCDIR)/rinex/rinex_downloader.c \
          $(SRCDIR)/sdr/sdr_interface.c \
          $(SRCDIR)/sdr/hackrf_interface.c \
          $(SRCDIR)/sdr/pluto_interface.c \
          $(SRCDIR)/sdr/bladerf_interface.c \
          $(SRCDIR)/sdr/usrp_interface.c \
          $(SRCDIR)/sdr/limesdr_interface.c \
          $(SRCDIR)/trajectory/motion_engine.c \
          $(SRCDIR)/signal/ca_code.c \
          $(SRCDIR)/signal/modulation.c \
          $(SRCDIR)/utils/math_utils.c \
          $(SRCDIR)/utils/time_utils.c \
          $(SRCDIR)/utils/config_parser.c

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Main executable
TARGET = $(BINDIR)/multi-gnss-sdr-sim

# Optional SDR support flags
HACKRF_SUPPORT = $(shell pkg-config --exists libhackrf && echo -DHACKRF_SUPPORT)
PLUTO_SUPPORT = $(shell pkg-config --exists libiio && echo -DPLUTO_SUPPORT)
BLADERF_SUPPORT = $(shell pkg-config --exists libbladeRF && echo -DBLADERF_SUPPORT)
USRP_SUPPORT = $(shell pkg-config --exists uhd && echo -DUSRP_SUPPORT)
LIMESDR_SUPPORT = $(shell pkg-config --exists LimeSuite && echo -DLIMESDR_SUPPORT)

# Optional SDR libraries
HACKRF_LIBS = $(shell pkg-config --exists libhackrf && pkg-config --libs libhackrf)
PLUTO_LIBS = $(shell pkg-config --exists libiio && pkg-config --libs libiio) $(shell pkg-config --exists libad9361 && pkg-config --libs libad9361)
BLADERF_LIBS = $(shell pkg-config --exists libbladeRF && pkg-config --libs libbladeRF)
USRP_LIBS = $(shell pkg-config --exists uhd && pkg-config --libs uhd)
LIMESDR_LIBS = $(shell pkg-config --exists LimeSuite && pkg-config --libs LimeSuite)

# Add optional flags and libraries
CFLAGS += $(HACKRF_SUPPORT) $(PLUTO_SUPPORT) $(BLADERF_SUPPORT) $(USRP_SUPPORT) $(LIMESDR_SUPPORT)
LDFLAGS += $(HACKRF_LIBS) $(PLUTO_LIBS) $(BLADERF_LIBS) $(USRP_LIBS) $(LIMESDR_LIBS)

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJDIR)/constellation $(OBJDIR)/rinex $(OBJDIR)/sdr $(OBJDIR)/trajectory $(OBJDIR)/signal $(OBJDIR)/utils $(BINDIR)

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete!"

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJDIR) $(BINDIR)

# Install (optional)
install: $(TARGET)
	@echo "Installing to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod 755 /usr/local/bin/multi-gnss-sdr-sim

# Uninstall (optional)
uninstall:
	@echo "Removing from /usr/local/bin..."
	sudo rm -f /usr/local/bin/multi-gnss-sdr-sim

# Test targets
test: $(TARGET)
	@echo "Running basic tests..."
	$(BINDIR)/multi-gnss-sdr-sim --version
	$(BINDIR)/multi-gnss-sdr-sim --help

test-gps: $(TARGET)
	@echo "Testing GPS constellation..."
	$(BINDIR)/multi-gnss-sdr-sim -e data/brdc0010.23n -l 30.286502,120.032669,100 -c gps -d 10 -o test_gps.bin

test-multi: $(TARGET)
	@echo "Testing multi-constellation..."
	$(BINDIR)/multi-gnss-sdr-sim -e data/brdc0010.23n -l 35.681298,139.766247,10 -c all -d 30 -o test_multi.bin

# Development targets
dev: CFLAGS += -g -DDEBUG -fsanitize=address
dev: LDFLAGS += -fsanitize=address
dev: clean all

# Release targets
release: CFLAGS += -DNDEBUG -march=native
release: clean all

# Documentation (if doxygen is available)
docs:
	@if command -v doxygen >/dev/null 2>&1; then \
		echo "Generating documentation..."; \
		doxygen Doxyfile; \
	else \
		echo "Doxygen not found. Install doxygen to generate documentation."; \
	fi

# Package source
dist:
	@echo "Creating source package..."
	tar -czf multi-gnss-sdr-sim-source.tar.gz \
		--exclude='*.o' --exclude='bin/*' --exclude='obj/*' \
		--exclude='.git' --exclude='*.tar.gz' \
		.

# Help target
help:
	@echo "Multi-GNSS SDR Simulator Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build the simulator (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  test         - Run basic tests"
	@echo "  test-gps     - Test GPS constellation"
	@echo "  test-multi   - Test multi-constellation"
	@echo "  dev          - Development build with debug flags"
	@echo "  release      - Optimized release build"
	@echo "  docs         - Generate documentation"
	@echo "  dist         - Create source package"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "SDR support detected:"
	@echo "  HackRF:      $(if $(HACKRF_SUPPORT),Yes,No)"
	@echo "  ADALM-Pluto: $(if $(PLUTO_SUPPORT),Yes,No)"
	@echo "  bladeRF:     $(if $(BLADERF_SUPPORT),Yes,No)"
	@echo "  USRP:        $(if $(USRP_SUPPORT),Yes,No)"
	@echo "  LimeSDR:     $(if $(LIMESDR_SUPPORT),Yes,No)"

# Phony targets
.PHONY: all directories clean install uninstall test test-gps test-multi dev release docs dist help

# Dependencies
$(OBJDIR)/multi_gnss_sim.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/constellation/gps.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/constellation/glonass.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/constellation/galileo.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/constellation/beidou.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/rinex/rinex_parser.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/rinex/rinex_downloader.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/sdr/sdr_interface.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/sdr/hackrf_interface.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/sdr/pluto_interface.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/sdr/bladerf_interface.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/sdr/usrp_interface.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/sdr/limesdr_interface.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/trajectory/motion_engine.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/signal/ca_code.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/signal/modulation.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/utils/math_utils.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/utils/time_utils.o: $(INCDIR)/multi_gnss_sim.h
$(OBJDIR)/utils/config_parser.o: $(INCDIR)/multi_gnss_sim.h