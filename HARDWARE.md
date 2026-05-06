# SumaShield Hardware Documentation

**Version:** 0.5

This document contains detailed hardware specifications, pinouts, bill of materials, and licensing information for SumaShield.

---

## Overview

SumaShield is a stackable PCB shield designed for Raspberry Pi Pico and Pico 2. It provides:

- I2S audio output (PT8211 DAC)
- TRS MIDI IN/OUT and audio output (3.5mm jack)
- OLED display interface (I2C)
- 8-key Cherry MX matrix
- Rotary encoder with push button
- 2x potentiometers (ADC input)
- 2x function buttons
- Optional microphone input (rear pad)
- Future NeoPixel support (GPIO 6, not populated in v0.5)

---

## Components

### Required Components

| Component | Description | Notes | Link |
|-----------|-------------|-------|------|
| **Host MCU** | | | |
| Raspberry Pi Pico | RP2040 board (Option A) | Original Pico board | [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) |
| Raspberry Pi Pico 2 | RP2350 board (Option B) | A4 stepping or later recommended | [Raspberry Pi Pico 2](https://www.raspberrypi.com/products/raspberry-pico-2/) |
| **Audio** | | | |
| PT8211 | I2S DAC 16-bit | Integrated on shield | [PT8211 Datasheet](https://datasheet.lcsc.com/lcsc/1912171634_Pulse-Junheng-Elec-PT8211_C189289.pdf) |
| **Display** | | | |
| OLED SSD1306 | 72x40 pixels (Option A - Recommended) | Less visual noise | [SSD1306 Datasheet](https://www.solomon-systech.com/files/default/catalogue%20archive/SSD1306%20oLED%20Driver%20IC%20Rev%201.1.pdf) |
| OLED SSD1306 | 0.96" 128x64 pixels (Option B) | May have visual noise | [SSD1306 Datasheet](https://www.solomon-systech.com/files/default/catalogue%20archive/SSD1306%20oLED%20Driver%20IC%20Rev%201.1.pdf) |
| **Controls** | | | |
| Cherry MX Switches | Mechanical key switches | 8x required | [Cherry MX](https://www.cherrymx.de/en/products) |
| Rotary Encoder | EC11 series | With push button | [EC11 Datasheet](https://datasheet.lcsc.com/lcsc/1811141824_Alps-Alpine-Electric-EC11E15244A3_C87793.pdf) |
| Push Buttons | 6x6mm tactile | 2x required | [TL3303 Datasheet](https://www.e-switch.com/system/asset/product_line/data_sheet/20/TL3303.pdf) |
| Potentiometers | 10kΩ linear | 2x required | - |
| **Connectors** | | | |
| TRS 3.5mm Jack | MJ-8435 type | MIDI TRS IN/OUT + Audio Out | [AKIZUKI MJ-8435](https://akizukidenshi.com/catalog/g/g109060/) |

### Optional Components

| Component | Description | Notes | Link |
|-----------|-------------|-------|------|
| **Microphone** | | | |
| Electret Mic Module | SparkFun BOB-12758 or compatible | Solder to ADC3 rear pad | [SparkFun BOB-12758](https://www.sparkfun.com/products/12758) |
| **LED** | | | |
| NeoPixel SK6812MINI-E | RGB LED | Not supported in v0.5, planned for future | [SK6812MINI-E Datasheet](https://cdn-shop.adafruit.com/product-files/4382/SK6812MINI-E+20200511.pdf) |

---

## Pin Configuration

### Pico/Pico 2 Pin Assignment

| Signal | GPIO | Direction | Description | Notes |
|--------|------|-----------|-------------|-------|
| **I2S Audio** | | | | |
| I2S DATA | 19 | Output → PT8211 | Audio data (LSBJ format) | - |
| I2S BCLK | 20 | Output → PT8211 | Bit clock | - |
| **I2C Display** | | | | |
| I2C SDA | 4 | Bidirectional | Display data | 400kHz I2C |
| I2C SCL | 5 | Output → Display | Display clock | - |
| **Key Matrix** | | | | |
| COL 0 | 12 | Output | Matrix column 0 | Driven LOW when scanning |
| COL 1 | 13 | Output | Matrix column 1 | Driven LOW when scanning |
| COL 2 | 14 | Output | Matrix column 2 | Driven LOW when scanning |
| COL 3 | 15 | Output | Matrix column 3 | Driven LOW when scanning |
| ROW 0 | 16 | Input | Matrix row 0 | Input pullup |
| ROW 1 | 17 | Input | Matrix row 1 | Input pullup |
| **Rotary Encoder** | | | | |
| ROT SW | 9 | Input | Push button | Input pullup |
| ROT CLK | 10 | Input | Clock signal | Interrupt, input pullup |
| ROT DT | 11 | Input | Data signal | Input pullup |
| **Buttons** | | | | |
| BTN A | 7 | Input | Button A | Input pullup |
| BTN B | 8 | Input | Button B | Input pullup |
| **ADC** | | | | |
| ADC 0 | 26 | Input | Potentiometer 0 | 12-bit SAR ADC |
| ADC 1 | 27 | Input | Potentiometer 1 | 12-bit SAR ADC |
| ADC 3 | 29 | Input | Microphone input | Rear pad, requires soldering |
| **MIDI** | | | | |
| UART TX | 0 | Output | MIDI OUT | 31250 baud |
| UART RX | 1 | Input | MIDI IN | 31250 baud |
| **LED (Future)** | | | | |
| NeoPin | 6 | Output | WS2812/SK6812 | Not supported in v0.5 |

---

## Electrical Specifications

### Power Requirements

| Parameter | Value | Notes |
|-----------|-------|-------|
| Input Voltage | 5V | From Pico USB |
| Current Draw | ~150mA typical | With audio output, OLED display |
| Max Current | ~250mA | All keys pressed, display on |

### Audio Output Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| DAC | PT8211 | 16-bit I2S |
| Output Impedance | < 1Ω | PT8211 direct output |
| Max Output Level | 1.0V RMS | Line-level |
| Frequency Response | 20Hz - 20kHz | @ 44.1kHz sample rate |
| SNR | > 90dB | Typical |

### MIDI Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| Interface | TRS 3.5mm Type A | Compatible with MIDI 1.0 |
| Baud Rate | 31250 bps | Standard MIDI |
| Current Loop | 5V via 220Ω resistor | MIDI OUT standard |

---

## Version History

### v0.5 (Current)
- Initial hardware release
- PT8211 I2S DAC
- OLED display support (72x40 recommended)
- 8-key Cherry MX matrix
- TRS MIDI IN/OUT (3.5mm MJ-8435)
- Rotary encoder, 2x pots, 2x buttons
- Optional microphone input (ADC3)
- Pico/Pico 2 compatible

---

## Open Source Hardware (OSHW) Statement

## Open Source Hardware (OSHW) Statement

This hardware design is released under the **Open Source Hardware (OSHW) Statement 1.0**.

The following hardware design files are distributed under the terms of the
Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0) license:

- Schematics
- PCB layouts
- Bill of materials
- Hardware documentation

You are free to:
- **Share**: Copy and redistribute the material in any medium or format
- **Adapt**: Remix, transform, and build upon the material

Under the following terms:
- **Attribution**: You must give appropriate credit to Hugelton instruments
- **ShareAlike**: If you remix, transform, or build upon the material, you must distribute your contributions under the same license

## Hardware License

**Creative Commons Attribution-ShareAlike 4.0 International**
https://creativecommons.org/licenses/by-sa/4.0/

The full local license text is included at `LICENSES/CC-BY-SA-4.0.txt`.

## Software License

The software (firmware, Arduino sketches, DSP code) is licensed under the MIT License.
See LICENSE file for details.

## Third-Party Components

### Suma Core (RT Audio Engine)
- **License**: GNU Lesser General Public License v2.1 (LGPL-2.1)
- **Repository**: https://github.com/kurogedelic/Suma
- **Description**: Related desktop real-time audio core whose architecture informs SumaShield. Suma Core source is not vendored in this repository.

## Attribution

When using this hardware design, please provide attribution to:

**Hugelton instruments**
- Creator: Leo Kuroshita
- GitHub: https://github.com/kurogedelic
- X: @kurogedelic

## Disclaimer

This hardware design is provided "as is", without warranty of any kind.
The creators are not liable for any damages arising from its use.
