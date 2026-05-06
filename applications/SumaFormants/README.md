# SumaFormants

**Monophonic formant vocoder synth with 8 vowel sounds**

**Version:** 1.0.0

## Overview

SumaFormants is a monophonic formant synthesizer that simulates human vowel sounds using three parallel resonant filters (formants). Based on the source-filter model of speech production, it creates realistic vowel sounds by filtering a rich source waveform (sawtooth + breath noise).

## Features

- **8 Vowel Sounds:** i, e, ɛ, a, ɑ, ɔ, o, u (IPA standard)
- **3 Formant Filters:** F1, F2, F3 per vowel (acoustically accurate)
- **Breath Control:** Mix between pure tone and white noise
- **Pitch Control:** Wide range (50-1000Hz) via ADC
- **Formant Offset:** Rotary encoder adjusts all formants (±100Hz)
- **Octave Drop:** Button A shifts pitch down 1 octave
- **Animated Face:** OLED shows mouth movement based on vowel
- **Dual MIDI:** USB + DIN MIDI input
- **Auto-sleep:** OLED turns off after 3 minutes

## Hardware Requirements

**SumaShield Platform v0.5 or later** (shield for Raspberry Pi Pico/Pico 2/Clones):
- Raspberry Pi Pico / Pico 2 / Clone boards
- **On-board I2S DAC:** PT8211 (integrated on shield)
- OLED SSD1306 (72x40)
- **Key Matrix:** 4x2 Cherry MX compatible sockets (8 keys)
- Rotary encoder
- **2x Push buttons** (6x6mm tactile)
- **2x Potentiometers** (10kΩ linear)
- **DIN MIDI Input** (via optoisolator, GPIO 1)

## Pin Configuration

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

### Prerequisites

1. Install Arduino IDE 2.x
2. Add Raspberry Pi Pico board support
3. Install libraries:
   - I2S (built-in for RP2040)
   - Wire (built-in)
   - U8g2 (OLED display)
   - Adafruit TinyUSB Library
   - MIDI Library by Forty Seven Effects
4. Set USB Stack to "TinyUSB"

### Upload

1. Open `SumaFormants.ino` in Arduino IDE
2. Select board: "Raspberry Pi Pico"
3. Set USB Stack: "TinyUSB"
4. Hold BOOTSEL button, plug Pico into USB
5. Click Upload

## Vowel Layout

| Key/MIDI | Vowel | IPA  | F1    | F2     | F3     |
|----------|-------|------|-------|--------|--------|
| 0 / C4   | i     | [i]  | 240Hz | 2400Hz | 3200Hz |
| 1 / D4   | e     | [e]  | 390Hz | 2300Hz | 3000Hz |
| 2 / E4   | ɛ     | [ɛ]  | 610Hz | 1900Hz | 2800Hz |
| 3 / F4   | a     | [a]  | 850Hz | 1610Hz | 2500Hz |
| 4 / G4   | ɑ     | [ɑ]  | 750Hz | 1090Hz | 2440Hz |
| 5 / A4   | ɔ     | [ɔ]  | 500Hz | 700Hz  | 2300Hz |
| 6 / B4   | o     | [o]  | 360Hz | 640Hz  | 2250Hz |
| 7 / C5   | u     | [u]  | 250Hz | 595Hz  | 2110Hz |

## Controls

### Button A: Octave Down

- **Press:** Lower pitch by 1 octave (-12 semitones)
- **Release:** Return to normal pitch
- **Held:** Pitch stays low until released

### Button B: Reserved

- No function (reserved for future features)

### Rotary Encoder

- **Rotation:** Adjust formant offset (±100Hz)
- **Range:** -100Hz to +100Hz
- **Center:** No offset (default formant frequencies)
- **Effect:** Shifts F1, F2, F3 frequencies for all vowels
- **Use:** Modify vowel character without changing keys

### Potentiometers

- **ADC 26 (Breath):** Noise mix amount (0-100%)
  - 0% = Pure tone (sawtooth)
  - 100% = Full noise (breath sound)

- **ADC 27 (Pitch):** Base pitch (50-1000Hz, exponential curve)
  - Low: 50Hz (bass)
  - High: 1000Hz (treble)

### Key Matrix (8 Keys)

Keys 0-7 trigger vowels i-u (see table above)

## Synthesis Engine

**Source:**
- Rich sawtooth wave (contains all harmonics)
- Optional breath: White noise (mixed by ADC 26)

**Formant Filters:**
- 3× Chamberlin State Variable Filters (SVF) in parallel
- F1, F2, F3 frequencies per vowel (IPA standard)
- Pre-computed coefficients for optimal performance
- Formant portamento: 50ms auto-glide on key change

**Voice Architecture:**
- Monophonic Last-Key Priority
- Only one voice active at a time
- Most recently pressed key takes priority
- Previous key envelope releases on new key press

**Envelope:**
- Attack: Fast (+0.02 per sample)
- Release: Medium (×0.985 per sample)
- Threshold: 0.001 for voice on/off

## MIDI Operation

### USB MIDI

1. Connect USB MIDI controller
2. Play notes C4-C5 to trigger vowels
3. Pitch follows MIDI note number (A4=440Hz formula)

**MIDI Pitch Formula:**
```
freq = 440 × 2^((note - 69) / 12)
```

### DIN MIDI

1. Connect MIDI device to GPIO 1 (via optoisolator)
2. Play notes C4-C5 to trigger vowels
3. Simultaneous use with USB MIDI

**MIDI Features:**
- Note On: Triggers vowel sound
- Note Off: Releases envelope
- Velocity: No function (fixed volume)
- Channel: Omni (responds to all channels)
- Pitch Bend: Not implemented

## OLED Display (72x40)

**Animated Face:**
- Eyes blink automatically (3-5 second intervals)
- Mouth movement synced to vowel
- Mouth opens wider for open vowels (a, ɑ, ɔ)

**Display Shows:**
- Current vowel symbol (IPA)
- Face animation
- Activity indicator
- Version info (on startup)

**Auto-sleep:**
- Display turns off after 3 minutes of inactivity
- Wake up on any input (key, MIDI, pot, button)
- Prevents OLED burn-in

## Specifications

**Audio:**
- Sample Rate: 22.05kHz
- Bit Depth: 16-bit
- Format: LSBJ (Least-Significant-Bit-Justified)
- Output: Mono (duplicated to L+R)

**Synthesis:**
- Source: Sawtooth wave + white noise
- Filters: 3× parallel SVF (Chamberlin)
- Formants: 3 resonant peaks per vowel
- Envelope: Attack/Release with threshold

**Performance:**
- CPU Usage: Dual-core (Core 0: UI, Core 1: Audio)
- Polyphony: Monophonic (last-key priority)
- Voice: Single voice with smooth transitions

## Formant Frequencies

Formant frequencies based on Peterson & Barney (1952):

| Vowel | IPA  | F1    | F2     | F3     | Quality         |
|-------|------|-------|--------|--------|-----------------|
| i     | [i]  | 240Hz | 2400Hz | 3200Hz | Bright, focused |
| e     | [e]  | 390Hz | 2300Hz | 3000Hz | Fronted        |
| ɛ     | [ɛ]  | 610Hz | 1900Hz | 2800Hz | Open-mid       |
| a     | [a]  | 850Hz | 1610Hz | 2500Hz | Open, resonant |
| ɑ     | [ɑ]  | 750Hz | 1090Hz | 2440Hz | Back, dark     |
| ɔ     | [ɔ]  | 500Hz | 700Hz  | 2300Hz | Mid-low        |
| o     | [o]  | 360Hz | 640Hz  | 2250Hz | Low-mid        |
| u     | [u]  | 250Hz | 595Hz  | 2110Hz | Low, dark      |

## Tips & Tricks

### Formant Transitions

- Play rapid key changes for smooth vowel transitions
- 50ms portamento creates realistic speech-like transitions
- No clicks or pops between vowels

### Breath Control

- Use ADC 26 for vocal expression
- 0% = Pure tone (clarinet-like)
- 50% = Balanced (voice-like)
- 100% = Full noise (whisper/breath)

### Pitch Control

- Use ADC 27 for wide pitch range (50-1000Hz)
- Use rotary for formant offset (±100Hz)
- Use Button A for bass drop

### MIDI Performance

- Play melody on MIDI keyboard
- Formants follow pitch
- Monophonic (last note priority)

## Troubleshooting

### No Sound Output

1. Check I2S wiring (GPIO 19, 20)
2. Verify PT8211 DAC power supply
3. Check sample rate (22.05kHz)
4. Try pressing keys to trigger sound

### No Formant Character

- If sound is like a plain sawtooth: Filters not working
- Check filter coefficients in code
- Verify formant frequency values

### No MIDI Response

1. Verify "TinyUSB" stack selected in Arduino IDE
2. Check DIN MIDI wiring (GPIO 1)
3. Test with MIDI monitoring software

### OLED Face Not Animating

1. Check I2C wiring (GPIO 4, 5)
2. Verify OLED address (0x3C)
3. Check 3.3V power supply

### Mouth Not Moving

- Verify key presses are registering
- Check current vowel is displayed
- Try different keys to see animation

## Technical Details

### Filter Implementation

Chamberlin State Variable Filter (SVF):
- 3 parallel filters (F1, F2, F3)
- Pre-computed coefficients
- Optimized for RP2040
- Q factor fixed for formant bandwidth

### Portamento

50ms linear interpolation between formant sets:
- Prevents clicks on key changes
- Creates smooth vowel transitions
- Applied to all formant frequencies

### Source Waveform

Rich sawtooth wave:
- Contains all harmonics
- Ideal for formant filtering
- Generated by `sinf()` with phase accumulator

## Version History

### Version 1.0.0 (2026-05-06)
- Initial release
- 8 vowel sounds with IPA notation
- Formant filters based on Peterson & Barney (1952)
- Breath control (noise mix)
- Wide pitch range (50-1000Hz)
- Octave drop button
- Rotary formant offset (±100Hz)
- Animated face on OLED
- Dual MIDI input (USB + DIN)

## Author

**SumaFormants** by [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)

**SumaShield** by [Hugelton instruments](https://github.com/hugelton)

## References

- [Peterson & Barney (1952)](https://www.acoustics.hut.fi/publications/files/theses/peterson-barney.pdf) - Control methods used in analysis, synthesis, and perception of speech
- [Chamberlin SVF](https://www.amazon.com/Geiger-Counter/dp/0393332621) - Musical Applications of Microprocessors (p. 89)
- [Formant Synthesis](https://en.wikipedia.org/wiki/Synthesis#Formant) - Wikipedia article on formant synthesis

## License

**Firmware:** MIT License

**Hardware:** Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)

See [LICENSE](../../LICENSE) and [HARDWARE.md](../../HARDWARE.md) for details.
