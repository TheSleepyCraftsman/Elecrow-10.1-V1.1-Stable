# Slave Firmware Binaries for LittleFS OTA

This directory should contain ESP32 slave firmware `.bin` files for LittleFS-based OTA updates.

## Usage

1. Copy your slave firmware binary (e.g., `network_adapter.bin`) to this directory
2. Build the project with `CONFIG_OTA_METHOD_LITTLEFS=y`
3. The build system will automatically:
   - Find the newest `.bin` file in this directory
   - Create a LittleFS partition image containing the firmware
   - Flash it to the `storage` partition

## Download

For the CrowPanel 7" ESP32-P4 C6 upgrade, download esp\_hosted v2.9.7:

```bash
curl -L -o network_adapter.bin \
    https://esphome.github.io/esp-hosted-firmware/v2.9.7/network_adapter_esp32c6.bin
```

Expected SHA256: `c9286c980b98b362c5b8862bf1d31bf6523e590cc68d70185b206d63f6c8bd11`

See the project [README](../../README.md) for alternative options (build from source).

## Important Notes

- Only `.bin` files are recognized
- If multiple files exist, the newest one (by timestamp) is used
- The firmware must be a valid ESP32 binary with proper image header
- This directory is only used during build time, not runtime

## Example

```bash
# Copy your slave firmware here
cp /path/to/your/slave/build/network_adapter.bin ./slave_fw_bin/

# Then build the project
idf.py -p <host_port> build flash monitor
```
