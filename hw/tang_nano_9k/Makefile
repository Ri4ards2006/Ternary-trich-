# =============================================================================
# Makefile for Tang Nano 9K (Gowin GW1NR-9C) using Open-Source EDA Toolchain
# Toolchain: Yosys -> NextPNR-Himbaechel -> Gowin_Pack (Apycula) -> openFPGALoader
# =============================================================================

# Toolchain detection (Support system PATH and oss-cad-suite installs)
OSS_CAD_PATHS ?= /home/richard/.local/oss-cad-suite/bin:/home/richard/Apps/Tools/oss-cad-suite/bin
export PATH := $(OSS_CAD_PATHS):$(PATH)

YOSYS         ?= yosys
NEXTPNR       ?= nextpnr-himbaechel
GOWIN_PACK    ?= gowin_pack
OPENFPGALOADER ?= openFPGALoader

# Target Hardware Architecture
DEVICE        := GW1NR-LV9QN88PC6/I5
FAMILY        := GW1N-9C
BOARD         := tangnano9k

# Source Files & Constraints
TOP_MODULE    := top
CST_FILE      := tangnano9k.cst
SRCS          := src/gowin_rpll.v \
                 src/uart_rx.v \
                 src/ternary_alu.v \
                 src/tmds_encoder.v \
                 src/dvi_transmitter.v \
                 src/video_gen.v \
                 src/top.v

BUILD_DIR     := build

# Output Artifacts
JSON_SYNTH    := $(BUILD_DIR)/$(TOP_MODULE).json
JSON_PNR      := $(BUILD_DIR)/$(TOP_MODULE)_pnr.json
BITSTREAM     := $(BUILD_DIR)/pack.fs

.PHONY: all synth pnr pack flash flash-sram clean

all: pack

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 1. Synthesis via Yosys
synth: $(JSON_SYNTH)

$(JSON_SYNTH): $(SRCS) | $(BUILD_DIR)
	@echo "========================================"
	@echo " [1/3] Synthesizing Verilog with Yosys"
	@echo "========================================"
	$(YOSYS) -p "read_verilog -sv $(SRCS); synth_gowin -top $(TOP_MODULE) -json $(JSON_SYNTH)"

# 2. Place & Route via nextpnr-himbaechel
pnr: $(JSON_PNR)

$(JSON_PNR): $(JSON_SYNTH) $(CST_FILE)
	@echo "========================================"
	@echo " [2/3] Place & Route with NextPNR"
	@echo "========================================"
	$(NEXTPNR) --json $(JSON_SYNTH) \
	           --write $(JSON_PNR) \
	           --device $(DEVICE) \
	           --vopt family=$(FAMILY) \
	           --vopt cst=$(CST_FILE)

# 3. Bitstream Generation via gowin_pack
pack: $(BITSTREAM)

$(BITSTREAM): $(JSON_PNR)
	@echo "========================================"
	@echo " [3/3] Generating Bitstream with gowin_pack"
	@echo "========================================"
	$(GOWIN_PACK) -d $(FAMILY) -o $(BITSTREAM) $(JSON_PNR)
	@echo "Bitstream generated: $(BITSTREAM)"

# 4. Programming Target
flash-sram: $(BITSTREAM)
	@echo "Loading bitstream into Tang Nano 9K SRAM..."
	$(OPENFPGALOADER) -b $(BOARD) $(BITSTREAM)

flash: $(BITSTREAM)
	@echo "Flashing bitstream into Tang Nano 9K Onboard Flash..."
	$(OPENFPGALOADER) -b $(BOARD) -f $(BITSTREAM)

clean:
	rm -rf $(BUILD_DIR)

