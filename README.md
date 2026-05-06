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

**SumaShield Platform (Shield Compatible with RP2040 Boards):**
- **Version:** 0.5
- Raspberry Pi Pico / Pico 2 / Clone boards
- **On-board I2S DAC:** PT8211 (integrated on shield)
- OLED SSD1306 (72x40)
- **Key Switches:** Cherry MX compatible (8 keys)
- Rotary encoder, 2x buttons, 2x pots

**Pin Configuration:**
```
I2S Audio:    GPIO 19 (DATA), GPIO 20 (BCLK) - PT8211 on shield
I2C Display:  GPIO 4 (SDA), GPIO 5 (SCL)
Key Matrix:   GPIO 12-15 (COL), GPIO 16-17 (ROW) - Cherry MX sockets
Rotary:       GPIO 9 (SW), GPIO 10 (CLK), GPIO 11 (DT)
Buttons:      GPIO 7 (A), GPIO 8 (B)
ADC:          GPIO 26, GPIO 27
MIDI DIN:     GPIO 1 (RX)
```

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
