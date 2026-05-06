# SumaShield

**SumaShield** is a collection of music performance applications for Raspberry Pi Pico (RP2040), featuring real-time DSP synthesis, dual-core architecture, and MIDI support.

## Applications

- **SumaDrums**: 8-track drum sequencer with step recording
- **SumaFormants**: Monophonic formant vocoder synth with 8 vowel sounds
- **MIDITester**: MIDI diagnostic tool for debugging reception issues

## Features

- Real-time DSP synthesis (22.05kHz, glitch-free)
- Dual-core architecture (UI + Audio)
- USB MIDI + DIN MIDI input
- OLED display (72x40) with auto-sleep
- Individual track parameters

## Hardware

**SumaShield Platform v0.5** - Shield for Raspberry Pi Pico/Pico 2/Clone boards

### Components

| Component | Required | Notes | Link |
|-----------|----------|-------|------|
| Raspberry Pi Pico | ✅ Yes | Pico/Pico 2/Clone compatible | [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) |
| PT8211 I2S DAC | ✅ Yes | Integrated on shield | [PT8211 Datasheet](https://datasheet.lcsc.com/lcsc/1912171634_Pulse-Junheng-Elec-PT8211_C189289.pdf) |
| OLED SSD1306 | ✅ Yes | 72x40 pixels, I2C interface | [SSD1306 Datasheet](https://www.solomon-systech.com/files/default/catalogue%20archive/SSD1306%20oLED%20Driver%20IC%20Rev%201.1.pdf) |
| Cherry MX Switches | ✅ Yes | 8 keys (4x2 matrix), user-installed | [Cherry MX](https://www.cherrymx.de/en/products) |
| Rotary Encoder | ✅ Yes | With push button | [EC11 Datasheet](https://datasheet.lcsc.com/lcsc/1811141824_Alps-Alpine-Electric-EC11E15244A3_C87793.pdf) |
| Push Buttons (2x) | ✅ Yes | 6x6mm tactile, momentary | [TL3303 Datasheet](https://www.e-switch.com/system/asset/product_line/data_sheet/20/TL3303.pdf) |
| Potentiometers (2x) | ✅ Yes | 10kΩ linear, for ADC input | - |
| MIDI DIN Connector | Optional | 5-pin DIN, via optoisolator (6N137) | [MIDI Spec](https://www.midi.org/specifications) |
| 6N137 Optoisolator | Optional | For DIN MIDI input | [6N137 Datasheet](https://datasheet.lcsc.com/lcsc/1811141824_Vishay-6N137_C80782.pdf) |

### Pin Configuration

| Signal | GPIO | Description | Notes |
|--------|------|-------------|-------|
| I2S DATA | 19 | PT8211 data input | LSBJ format |
| I2S BCLK | 20 | PT8211 bit clock | - |
| I2C SDA | 4 | OLED display data | 400kHz I2C |
| I2C SCL | 5 | OLED display clock | - |
| COL 0-3 | 12-15 | Key matrix columns | Output |
| ROW 0-1 | 16-17 | Key matrix rows | Input pullup |
| ROT SW | 9 | Rotary encoder button | Input pullup |
| ROT CLK | 10 | Rotary encoder clock | Interrupt |
| ROT DT | 11 | Rotary encoder data | Interrupt |
| BTN A | 7 | Button A | Input pullup |
| BTN B | 8 | Button B | Input pullup |
| ADC 0 | 26 | Potentiometer 0 | 12-bit SAR ADC |
| ADC 1 | 27 | Potentiometer 1 | 12-bit SAR ADC |
| MIDI RX | 1 | DIN MIDI receive | 31250 baud UART |

## Quick Start

1. Install Arduino IDE 2.x
2. Add Raspberry Pi Pico board support
3. Install libraries: U8g2, Adafruit TinyUSB Library, MIDI
4. Set USB Stack to "TinyUSB"
5. Open `.ino` file and upload

## Requirements

**Libraries:**
- I2S (built-in for RP2040)
- Wire (built-in)
- U8g2 (OLED display)
- Adafruit TinyUSB Library
- MIDI Library by Forty Seven Effects

**Arduino IDE Settings:**
- Board: Raspberry Pi Pico
- USB Stack: TinyUSB

## License

**Firmware source and software documentation:** MIT License - See [LICENSE](LICENSE) for details.

**Hardware design files and hardware-specific documentation:** Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0).
This applies to the KiCad design files, manufacturing outputs under `hardware/`, and hardware documentation.
See [HARDWARE.md](HARDWARE.md) and [LICENSES/CC-BY-SA-4.0.txt](LICENSES/CC-BY-SA-4.0.txt) for details.

**External dependencies:** Arduino core and libraries are not vendored in this repository and remain under their upstream licenses.
Suma Core is referenced as a related project under GNU Lesser General Public License v2.1 (LGPL-2.1); see [LICENSES/LGPL-2.1.txt](LICENSES/LGPL-2.1.txt).

---

**Author:** [Hugelton instruments](https://github.com/kurogedelic)  
**Creator:** [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)

## Hardware Version History

### v0.5 (Current)
- Initial hardware release
- PT8211 I2S DAC (on-board)
- 72x40 OLED display
- 8-key Cherry MX matrix
- Dual MIDI input (USB + DIN)
- Rotary encoder with button
- 2 potentiometers
- 2 function buttons
