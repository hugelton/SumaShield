# SumaShield


**An open-source audio/MIDI shield platform for Raspberry Pi Pico / Pico 2**


<img width="3545" height="2541" alt="IMG_6342" src="https://github.com/user-attachments/assets/e0e2c0d3-acbf-4e0f-bcbe-503bd82bd1a1" />

SumaShield is a stackable shield designed for the Raspberry Pi Pico and Pico 2, providing dedicated I/O for real-time audio synthesis, MIDI communication, and user interfaces. Created as a reusable embedded DSP experimentation platform, SumaShield enables developers to build custom synthesizers, sequencers, effects processors, and experimental audio instruments.

Inspired by the ClockworkPi PicoCalc ecosystem, SumaShield leverages the existing Pico development toolchain while adding specialized audio/MIDI hardware on top. The platform is open-source hardware (CC BY-SA 4.0), with firmware examples released under MIT license.

---

## Features

- **Shield Platform**: Stackable design for Raspberry Pi Pico / Pico 2
- **Audio Output**: PT8211 16-bit DAC (LSB Justified format, not standard I2S)
- **MIDI TRS**: 3.5mm jack supporting MIDI IN/OUT (Type A specification)
- **Audio Output**: Line-level output via same 3.5mm TRS jack
- **Display Support**: OLED SSD1306 (72x40 recommended, 128x64 compatible)
- **User Input**: 8-key Cherry MX matrix, rotary encoder, 2 potentiometers
- **Development**: Arduino IDE compatible, uses existing RP2040/RP2350 toolchain
- **Expandable**: Optional microphone input, future NeoPixel support planned

---

## Why SumaShield?

"Want to make sound with Pico, but breadboarding is a hassle and existing shields are pricey..."

SumaShield aims to be a simple, affordable audio shield you can quickly grab and start using.

This project is all about skipping the complicated stuff and just having fun with audio programming on Pico!

---

### What SumaShield Is

SumaShield is designed as a **platform**, not a closed appliance. Unlike commercial synthesizers or audio gadgets, SumaShield assumes you will:

- **Write your own firmware** - Use Arduino IDE, Pico SDK, or any RP2040/RP2350 development toolchain
- **Design your own DSP** - Implement synthesis, sequencing, effects, or experimental audio algorithms
- **Define your own UI** - The hardware provides controls; you decide what they do
- **Learn and experiment** - Real-time DSP on a microcontroller is educational and fun

The included applications (SumaDrums, SumaFormants, MIDITester) are **examples**, not the primary purpose. They demonstrate what's possible and serve as starting points for your own projects.

---

## Example Applications

The repository includes several firmware examples demonstrating different approaches to audio/MIDI on SumaShield:

### [SumaDrums](applications/SumaDrums/)
8-track drum sequencer with step recording, individual track parameters, and pattern storage.

### [SumaFormants](applications/SumaFormants/)
Monophonic formant vocoder synth simulating human vowel sounds using three parallel resonant filters.

### [PixelSynth](applications/PixelSynth/)
16x16 LED matrix visual synth with 6 procedural animation modes. *(Requires NeoPixel GPIO 6 - not in v0.5)*

### [MIDITester](applications/MIDITester/)
MIDI diagnostic tool for debugging USB and DIN MIDI reception issues.

### [NeopixelTest](applications/NeopixelTest/)
WS2812 RGB LED test utility (for future NeoPixel support).

---

## Hardware

SumaShield is a custom PCB that stacks onto Raspberry Pi Pico/Pico 2:

**Required Components:**
- Raspberry Pi Pico (RP2040) or Pico 2 (RP2350, A4+ stepping recommended)
- PT8211 16-bit DAC (on-board, LSB Justified format)
- OLED SSD1306 display (72x40 recommended)
- 8x Cherry MX switches
- 3.5mm TRS jack (MJ-8435 type)
- Rotary encoder, 2x push buttons, 2x potentiometers

**Optional Components:**
- Electret microphone module (SparkFun BOB-12758, rear pad ADC3)

**For detailed hardware specifications, pinouts, and BOM, see [HARDWARE.md](HARDWARE.md)**

---

## Quick Start

### Prerequisites

1. **Hardware**: Assemble SumaShield with Raspberry Pi Pico/Pico 2
2. **Software**: Arduino IDE 2.x with Pico board support
3. **Libraries**: Install required libraries (U8g2, Adafruit TinyUSB, MIDI)
4. **Settings**: Set USB Stack to "TinyUSB"

### Upload Example Firmware

```bash
# Flash example application
cd applications/SumaDrums
# Open SumaDrums.ino in Arduino IDE
# Upload to Pico
```

### Develop Your Own Firmware

```cpp
// Basic I2S output example
#include <I2S.h>

I2S i2s(OUTPUT);

void setup1() {
  i2s.setBCLK(20);
  i2s.setDATA(19);
  i2s.begin(22050);  // 22.05kHz sample rate
}

void loop1() {
  // Your DSP code here
  int16_t sample = generate_sample();
  i2s.write(sample);
  i2s.write(sample);  // Stereo
}
```

For complete examples, see the `applications/` directory.

---

## Specifications

| Category | Specification |
|----------|--------------|
| **Host** | Raspberry Pi Pico (RP2040) or Pico 2 (RP2350) |
| **Audio DAC** | PT8211 (16-bit, LSB Justified format) |
| **Display** | SSD1306 OLED (I2C, 72x40 or 128x64) |
| **MIDI** | TRS 3.5mm Type A (IN/OUT), USB MIDI support |
| **Audio Out** | Line-level via TRS jack (shared with MIDI) |
| **Controls** | 8-key matrix, rotary encoder, 2x ADC pots, 2x buttons |
| **Expandability** | Mic input (ADC3), NeoPixel (GPIO 6, future support) |
| **Power** | 5V via USB (from Pico) |
| **Form Factor** | Stackable shield, compatible with Pico/Pico 2 pinout |

---

## Development

SumaShield firmware can be developed using:

- **Arduino IDE** (Recommended for beginners)
- **Pico SDK C/C++** (For advanced users)
- **MicroPython/CircuitPython** (Possible, not yet documented)

All examples use Arduino IDE for accessibility. Dual-core architecture (Core 0: UI, Core 1: Audio) is recommended for glitch-free audio, but not required.

**Required Libraries:**
- I2S (built-in for RP2040)
- Wire (built-in)
- U8g2 (OLED display)
- Adafruit TinyUSB Library (USB MIDI)
- MIDI Library by Forty Seven Effects

---

## Hardware Design Files

Complete KiCad project sources are available in the `hardware/` directory:

- Schematic
- PCB layout
- BOM (Bill of Materials)
- Manufacturing outputs

Licensed under **Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)**.

---

## License

**Firmware source and software documentation:** [MIT License](LICENSE)

**Hardware design files and hardware-specific documentation:** [Creative Commons BY-SA 4.0](LICENSES/CC-BY-SA-4.0.txt)

This separation means:
- You can freely use/modify firmware in closed-source projects
- Hardware modifications must be shared under the same license (CC BY-SA 4.0)
- External dependencies (Arduino core, libraries) remain under their upstream licenses

---

## Resources

- [Hardware Documentation](HARDWARE.md)
- [Application Examples](applications/)
- [Contributing Guidelines](CONTRIBUTING.md)
- [Changelog](CHANGELOG.md)

---

## Acknowledgments

SumaShield is inspired by:
- [ClockworkPi PicoCalc](https://github.com/clockworkpi/BoardPackV2) - Pico-as-platform philosophy
- [pikocore](https://github.com/schollz/pikocore) - Lo-fi audio mangler architecture
- [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pico-pico/) - Compute platform

---

## Hardware Version History

### v0.5 (Current)
- Initial hardware release
- Supports Pico (RP2040) and Pico 2 (RP2350, A4+ recommended)
- PT8211 I2S DAC on-board
- OLED display support (72x40 recommended, 128x64 compatible)
- MIDI TRS via 3.5mm jack (MJ-8435)
- 8-key Cherry MX matrix
- Optional microphone input (ADC3 rear pad)
- Open-source hardware (CC BY-SA 4.0)

---

**Platform Design:** [Hugelton instruments](https://github.com/hugelton)

**Firmware Examples:** [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)
