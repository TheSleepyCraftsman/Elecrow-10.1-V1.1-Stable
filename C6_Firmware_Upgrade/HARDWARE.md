# CrowPanel 7" ESP32-P4 — C6 Hardware Reference

Hardware details for the ESP32-C6-MINI-1 co-processor on the CrowPanel Advanced 7" ESP32-P4 HMI Display (1024x600), PCB V1.0.

Source: Eagle schematic `ESP32-P4 Display 7.0 inch V1.0.sch` from the [Elecrow GitHub repo](https://github.com/Elecrow-RD/CrowPanel-Advanced-7inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen).

## C6 Connection Summary

The ESP32-C6-MINI-1 (schematic IC1) has **eight signal connections** to the board:

- 6 SDIO lines to the ESP32-P4 (data + clock + command)
- 1 reset/enable line to the P4 (GPIO32 → C6 EN)
- 1 strapping pullup (IO8 to 3.3V via R87)

Plus **three test-pad-only connections** (not routed to any connector or processor):

- UART TX (test pad P25)
- UART RX (test pad P21)
- Boot select (test pad P36)

USB, JTAG, and SPI slave pins connect to nothing.

## SDIO Bus (P4 ↔ C6)

| P4 GPIO | Net Name | C6 GPIO | Function | Series R |
|---------|----------|---------|----------|----------|
| GPIO18 | SD2_CLK | IO19 | SDIO CLK | R81 |
| GPIO19 | SD2_CMD | IO18 | SDIO CMD | R82 |
| GPIO14 | SD2_D0 | IO20 | Data 0 | R83 |
| GPIO15 | SD2_D1 | IO21 | Data 1 | R84 |
| GPIO16 | SD2_D2 | IO22 | Data 2 | R85 |
| GPIO17 | SD2_D3 | IO23 | Data 3 | R86 |

Series resistors and pullups to C6\_VDD\_3V3 terminate all data lines. The factory firmware uses **1-bit mode** (D0 only). Wiring and termination exist for all four data lines — **4-bit mode is electrically viable** but untested for OTA.

## Control Signals

| P4 GPIO | Net Name | C6 Pin | Function |
|---------|----------|--------|----------|
| GPIO32 | C6_EN | EN | Reset — **active high** (R77 pullup to 3.3V, C15 decoupling) |

Drive GPIO32 low to hold C6 in reset. Release (or drive high) to let it boot. The R77 pullup means the C6 runs by default when powered.

## Strapping Pins

| Net | C6 Pin | Connection | Boot Mode |
|-----|--------|------------|-----------|
| C6_IO8 | IO8 | Pulled up to 3.3V via R87 | Required high for download mode |
| C6_IO9_BOOT | IO9 | Test pad P36 only | LOW = download mode, HIGH = normal boot (45 kΩ internal pullup) |

GPIO8 already reads high (correct). Only GPIO9 needs external control to enter download mode.

## Test Pads

```
        ┌──────────────────────┐
        │   ESP32-C6-MINI-1    │
        │       (IC1)          │
        │                      │
        └──────────────────────┘
              ↓   ↓   ↓   ↓
            P25  P21  P36  P37
            TX   RX  BOOT  3V3
```

| Pad | Net Name | C6 Pin | Function | Wire Color |
|-----|----------|--------|----------|------------|
| P25 | C6_TXD0 | GPIO16 (TXD0) | UART transmit | Green |
| P21 | C6_RXD0 | GPIO17 (RXD0) | UART receive | White |
| P36 | C6_IO9_BOOT | GPIO9 | Boot select (LOW = download mode) | Yellow |
| P37 | C6_VDD_3V3 | — | 3.3V power reference | Red |

Schematic coordinates cluster at x≈-99, y≈510–521, near the C6 module. Pads are `TEST_PAD*` type footprints — small round copper pads, possibly labeled in silkscreen.

> **Physical location not verified from schematic alone.** Open the panel and look near the C6 module. The RF shield or solder mask may hide the pads.

### UART Wiring (for test pad flash)

| Test Pad | Connect To | Notes |
|----------|------------|-------|
| P25 (C6 TX) | USB-UART adapter **RX** | C6 transmits, adapter receives |
| P21 (C6 RX) | USB-UART adapter **TX** | Adapter transmits, C6 receives |
| P36 (BOOT) | GND during reset only | Hold low, reset C6, release after esptool connects |
| Board GND | USB-UART adapter **GND** | Use J7 pin 7/8 or a nearby ground via |

Do **not** connect the adapter's 3.3V output to P37. The C6 draws power from the board's own regulator.

### Download Mode Entry

1. Connect wires to P25, P21, and a GND point
2. Short P36 to GND (hold)
3. Reset the C6 — power-cycle the board or toggle GPIO32 from the P4
4. Wait for esptool to print `Connecting...`
5. Release P36

## C6 Partition Layout

From `esp-hosted-mcu/slave/partitions.esp32c6.csv`:

| Name | Type | Offset | Size |
|------|------|--------|------|
| nvs | data (nvs) | 0x9000 | 16 KB |
| otadata | data (ota) | 0xD000 | 8 KB |
| phy_init | data (phy) | 0xF000 | 4 KB |
| ota_0 | app (ota_0) | 0x10000 | 1920 KB |
| ota_1 | app (ota_1) | 0x1F0000 | 1920 KB |

Two OTA slots, no factory partition. OTA writes target the **inactive** slot; the active slot survives a failed transfer. `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` defaults to **disabled** — a successfully activated but crashing image enters a reboot loop with no automatic recovery.

> **Flash addresses:** ESP32-C6 bootloader goes at **0x0** (RISC-V), not 0x1000 (Xtensa ESP32). The app goes at 0x10000.

## SD Card Slot (Independent Bus)

| J5 Pin | P4 GPIO | Function |
|--------|---------|----------|
| SCLK | GPIO43 | SD clock |
| DI | GPIO44 | SD command |
| DO | GPIO39 | SD data 0 |

The SD card uses different P4 GPIOs from the C6 SDIO bus. Both operate simultaneously without conflict.

## SPI Connector J9

| Pin | P4 GPIO | Function |
|-----|---------|----------|
| 2 | GPIO8 | SPI CLK |
| 3 | GPIO7 | SPI MISO |
| 4 | GPIO6 | SPI MOSI |
| 5 | — | 3.3V |
| 6 | — | GND |
| 7 | — | 5V |

Designed for wireless modules (SX1262, nRF24). Also supports a W5500 SPI Ethernet module as a WiFi bypass if the C6 upgrade fails entirely.

## Connectors Not Connected to C6

These connectors are P4-only — none reach the C6:

| Connector | Function | P4 GPIOs |
|-----------|----------|----------|
| J2 | Crowtail UART | GPIO47 (TX), GPIO48 (RX) |
| J10 | External UART (level-shifted) | GPIO33 (RX), GPIO34 (TX) |
| J7 | 40-pin GPIO header | GPIO2–5, 25, 27–28, 49–52 |
| J9 | SPI / wireless module | GPIO6–8 |
| J11 | Wireless module (secondary) | GPIO9–10 |

## SDIO Configuration Reference

Working `sdkconfig` values for the CrowPanel C6 SDIO interface:

```ini
CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE=y
CONFIG_ESP_HOSTED_IDF_SLAVE_TARGET="esp32c6"
CONFIG_ESP_HOSTED_SDIO_SLOT_1=y
CONFIG_ESP_HOSTED_SDIO_1_BIT_BUS=y
CONFIG_ESP_HOSTED_SDIO_BUS_WIDTH=1
CONFIG_ESP_HOSTED_SDIO_CLOCK_FREQ_KHZ=10000
CONFIG_ESP_HOSTED_SDIO_RESET_ACTIVE_HIGH=y
CONFIG_ESP_HOSTED_SDIO_GPIO_RESET_SLAVE=32
CONFIG_ESP_HOSTED_PRIV_SDIO_PIN_CMD_SLOT_1=19
CONFIG_ESP_HOSTED_PRIV_SDIO_PIN_CLK_SLOT_1=18
CONFIG_ESP_HOSTED_PRIV_SDIO_PIN_D0_SLOT_1=14
CONFIG_ESP_HOSTED_PRIV_SDIO_PIN_D1_1BIT_BUS_SLOT_1=15
CONFIG_ESP_HOSTED_SDIO_OPTIMIZATION_RX_STREAMING_MODE=y
CONFIG_ESP_HOSTED_SLAVE_RESET_ON_EVERY_HOST_BOOTUP=y
CONFIG_ESP_HOSTED_SDIO_RESET_DELAY_MS=1500
```

The factory firmware uses 40 MHz clock. This project uses **10 MHz** for OTA reliability — proven stable for the full ~15-second transfer.
