# CrowPanel C6 SDIO OTA — task runner
# Upgrades ESP32-C6 co-processor firmware via SDIO from ESP32-P4

set dotenv-load := false
set shell := ["bash", "-euo", "pipefail", "-c"]

# Serial port for CrowPanel CH341 UART bridge (upper USB-C)
serial_port := `ls /dev/cu.wchusbserial* 2>/dev/null | head -1 || echo ""`
baud := "460800"

# Firmware binary location
fw_dir := "components/ota_littlefs/slave_fw_bin"

# --- Preflight ---

# Verify ESP-IDF environment is sourced
[private]
idf-check:
    @command -v idf.py >/dev/null 2>&1 || { \
        echo "ERROR: idf.py not found. Run: source \$IDF_PATH/export.sh"; exit 1; \
    }

# Verify esptool.py is available (works standalone or via IDF)
[private]
esptool-check:
    @command -v esptool.py >/dev/null 2>&1 || { \
        echo "ERROR: esptool.py not found. Install: pip install esptool (or source IDF)"; exit 1; \
    }

# Verify serial port is connected
[private]
port-check:
    @[ -n "{{serial_port}}" ] || { \
        echo "ERROR: No CH341 serial port found. Check USB-C connection (upper port)."; exit 1; \
    }
    @echo "Serial port: {{serial_port}}"

# --- Setup ---

# Verify C6 firmware binary is in place
check-fw:
    @FW=$(find {{fw_dir}} -name "*.bin" 2>/dev/null | head -1); \
    if [ -z "$FW" ]; then \
        echo "ERROR: No .bin file in {{fw_dir}}/"; \
        echo "Fix: cp /path/to/network_adapter.bin {{fw_dir}}/"; \
        exit 1; \
    fi; \
    echo "Firmware: $FW ($(wc -c < "$FW" | tr -d ' ') bytes)"

# Inspect C6 firmware binary (chip, flash mode, version)
verify-fw: esptool-check check-fw
    @FW=$(find {{fw_dir}} -name "*.bin" 2>/dev/null | head -1); \
    echo "--- Binary: $FW ---"; \
    esptool.py image_info --version 2 "$FW"

# Copy ota_littlefs component from downloaded esp_hosted (run after set-target)
fix-components: idf-check
    @HOSTED=$(find managed_components -name "espressif__esp_hosted" -type d 2>/dev/null | head -1); \
    if [ -z "$HOSTED" ]; then \
        echo "ERROR: managed_components not found. Run: just set-target"; exit 1; \
    fi; \
    OTA_SRC="$HOSTED/examples/host_performs_slave_ota/components/ota_littlefs"; \
    KCONFIG_SRC="$HOSTED/examples/host_performs_slave_ota/main/Kconfig.projbuild"; \
    if [ ! -d "$OTA_SRC" ]; then \
        echo "ERROR: ota_littlefs not found in esp_hosted examples"; exit 1; \
    fi; \
    echo "Copying ota_littlefs component..."; \
    cp -r "$OTA_SRC" components/; \
    if [ -f "$KCONFIG_SRC" ]; then \
        cp "$KCONFIG_SRC" main/; \
        echo "Copied Kconfig.projbuild"; \
    fi; \
    echo "Done. Preserving firmware binary in slave_fw_bin/."; \
    mkdir -p components/ota_littlefs/slave_fw_bin; \
    touch components/ota_littlefs/slave_fw_bin/.gitkeep

# --- Build ---

# Set IDF target to ESP32-P4 (downloads managed components)
set-target: idf-check
    idf.py set-target esp32p4

# Build the OTA flasher app
build: idf-check check-fw
    idf.py build

# Clean build artifacts
clean: idf-check
    idf.py fullclean

# --- Flash ---

# Flash OTA app to P4 and open serial monitor
flash: build port-check
    idf.py -p {{serial_port}} flash monitor

# Flash OTA app without opening monitor
flash-only: build port-check
    idf.py -p {{serial_port}} flash

# Open serial monitor (no flash)
monitor: idf-check port-check
    idf.py -p {{serial_port}} monitor

# --- Backup & Restore ---

# Backup full P4 flash (16 MB) before flashing OTA app
backup: esptool-check port-check
    @BACKUP="p4_backup_$(date +%Y%m%d_%H%M%S).bin"; \
    echo "Reading 16 MB from P4 flash to $BACKUP ..."; \
    echo "Warning: esptool read_flash can be unreliable on CrowPanel. Backup is best-effort."; \
    esptool.py --chip esp32p4 --port {{serial_port}} --baud {{baud}} \
        read_flash 0x0 0x1000000 "$BACKUP" && \
    echo "Backup saved: $BACKUP ($(wc -c < "$BACKUP" | tr -d ' ') bytes)" || \
    echo "Backup failed — proceed anyway if you can rebuild your firmware from source."

# --- Status ---

# Show project status (serial port, firmware binary, IDF version)
status:
    @echo "=== C6 SDIO OTA Status ==="
    @if command -v idf.py >/dev/null 2>&1; then \
        echo "ESP-IDF: $(idf.py --version 2>&1)"; \
    else \
        echo "ESP-IDF: not sourced (run: source \$IDF_PATH/export.sh)"; \
    fi
    @if [ -n "{{serial_port}}" ]; then \
        echo "Serial:  {{serial_port}}"; \
    else \
        echo "Serial:  not connected"; \
    fi
    @FW=$(find {{fw_dir}} -name "*.bin" 2>/dev/null | head -1); \
    if [ -n "$FW" ]; then \
        echo "Binary:  $FW ($(wc -c < "$FW" | tr -d ' ') bytes)"; \
    else \
        echo "Binary:  MISSING — place .bin in {{fw_dir}}/"; \
    fi
    @if [ -d "build" ]; then \
        echo "Build:   exists"; \
    else \
        echo "Build:   not built yet"; \
    fi

# List available recipes
[private]
default:
    @just --list
