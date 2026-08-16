# Elecrow ESP32-P4 10.1" Zigbee Gateway

Zigbee gateway setup for the Elecrow CrowPanel Advanced 10.1" ESP32-P4 board.
Uses the onboard **ESP32-H2** wireless socket as a Zigbee Coordinator RCP.

## Hardware Architecture

```
[ESP32-P4 Host]  ←─ SDIO ─→  [ESP32-C6]  (Wi-Fi / BT co-processor)
[ESP32-P4 Host]  ←─ UART ─→  [ESP32-H2]  (Zigbee RCP co-processor)
```

## Wireless Socket Pin Map (Elecrow V1.1 Schematic)

| Signal         | P4 GPIO | Direction   | Notes                        |
|----------------|---------|-------------|------------------------------|
| SPI2_MOSI      | 6       | P4 → Module | SPI data out                 |
| SPI2_MISO      | 7       | Module → P4 | SPI data in                  |
| SPI2_SCK       | 8       | P4 → Module | SPI clock                    |
| IO9_W          | 9       | P4 → H2     | **H2 BOOT** strapping pin    |
| IO10_W_BUSY    | 10      | Bidir       | H2 IO10                      |
| I2C1_SDA_3V3   | 45      | Bidir       | I2C data (shared bus)        |
| I2C1_SCL_3V3   | 46      | P4 → Module | I2C clock (shared bus)       |
| P4_TXD2        | 53      | P4 → H2     | **UART TX** → H2 RX          |
| P4_RXD2        | 54      | H2 → P4     | **UART RX** ← H2 TX          |
| EN             | —       | —           | Not wired (pulled to 3.3V)   |

## Project Structure

```
Elecrow-Zigbee-Gateway/
├── README.md               ← This file
├── h2_rcp/                 ← ESP32-H2 Zigbee RCP firmware
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c
└── p4_gateway/             ← ESP32-P4 Zigbee gateway host
    ├── CMakeLists.txt
    ├── sdkconfig.defaults
    └── main/
        ├── CMakeLists.txt
        └── main.c
```

## Build Order

### Step 1: Build and Flash H2 RCP Firmware

```powershell
cd C:\ESPProjects\Elecrow-Zigbee-Gateway\h2_rcp
. C:\esp\v5.5\esp-idf\export.ps1
idf.py set-target esp32h2
idf.py build
idf.py -p COMX flash    # Replace COMX with the H2's COM port
```

> **Note:** To flash the H2, you must access it via its own USB/UART port, or
> temporarily wire a USB-UART adapter to GPIO 53/54 with GPIO 9 held LOW.

### Step 2: Build and Flash P4 Host

```powershell
cd C:\ESPProjects\Elecrow-Zigbee-Gateway\p4_gateway
idf.py set-target esp32p4
idf.py build
idf.py -p COM6 flash    # Standard P4 flash port
```

With `CONFIG_ZIGBEE_GW_AUTO_UPDATE_RCP=y`, the P4 will automatically
re-flash the H2 if the RCP firmware version does not match.

## Integration with Factory Firmware

The Zigbee gateway code can be integrated into the main factory firmware
(`ESP32-P4-Adcance-10.1`) by adding the sdkconfig.defaults entries from
`p4_gateway/sdkconfig.defaults` into the factory project's sdkconfig.defaults,
and including the gateway component in the build.
