# SumaPiko

**Lo-fi music mangler adapted from pikocore for SumaShield hardware**

**Version:** 1.0.0

**⚠️ STATUS: NOT WORKING - Currently under development**

This application uploads successfully but does not produce audio output. The I2S conversion from PWM audio is still being debugged. DO NOT USE - consider using SumaDrums or SumaFormants instead.

Based on [pikocore](https://github.com/schollz/pikocore) by schollz, adapted for SumaShield hardware by Leo Kuroshita (Hugelton instruments).

## Overview

SumaPiko is a sample-based lo-fi music mangler that plays, chops, and mangles audio samples in real-time. It features:

- **Sample Playback**: Plays audio samples from flash memory with real-time manipulation
- **Beat Slicing**: Automatically slices samples into beats
- **Probability Mutations**: Random jumps, gates, direction changes, and tunnels
- **Retriggering**: 16 different retrigger patterns with pitch/volume/filter modulation
- **Effects**: Distortion, low-pass filter, noise gate
- **Crossfading**: Smooth transitions between sample sections
- **MIDI Sync**: DIN MIDI input for clock, notes, start/stop

## Hardware Requirements

**SumaShield Platform v0.5 or later** (shield for Raspberry Pi Pico/Pico 2/Clones):
- Raspberry Pi Pico / Pico 2 / Clone boards
- **On-board I2S DAC:** PT8211 (integrated on shield)
- OLED SSD1306 (72x40)
- **Key Matrix:** 4x2 Cherry MX compatible sockets (8 keys)
- **Rotary encoder**
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
ADC:          GPIO 26 (Knob 0), GPIO 27 (Knob 1)
MIDI DIN:     GPIO 1 (RX) - 31250 baud
```

## Quick Start

### Prerequisites

1. Install Arduino IDE 2.x
2. Add Raspberry Pi Pico board support
3. Install libraries:
   - I2S (built-in for RP2040)
   - Wire (built-in)
   - U8g2 (OLED display)
   - MIDI Library by Forty Seven Effects
4. Set USB Stack to "TinyUSB"

### Building Audio Samples

SumaPiko uses audio samples embedded in flash memory. To convert your own samples:

```bash
cd audio2h

# Add your FLAC/WAV/MP3 files to the demo/ folder

# Generate audio2h.h (adjust parameters as needed)
go run main.go -folder-in demo -folder-out converted -sr 31000 -bpm 165 -limit 1

# This generates doth/audio2h.h with embedded samples
```

**Parameters:**
- `-sr`: Sample rate (31000 Hz for lo-fi character)
- `-bpm`: Target BPM for time stretching
- `-limit`: Maximum number of samples to process

### Upload

1. Open `SumaPiko.ino` in Arduino IDE
2. Select board: "Raspberry Pi Pico"
3. Set USB Stack: "TinyUSB"
4. Hold BOOTSEL button, plug Pico into USB
5. Click Upload

## Controls

### Key Matrix (8 keys)

The 8 keys correspond to 8 beats in the current sample:

| Key | Beat | Function |
|-----|------|----------|
| 0-7 | Beat 0-7 | Hold to lock to that beat |
| 0+1 (hold both) | Retrigger | Trigger retrigger effect |

Pressing a key during playback locks the sequencer to that beat.

### Knobs

**Knob 0 (ADC 26):**
- When nothing held: Function selector (cycles through 8 functions)
- When key held: Function B (depends on selected function)

**Knob 1 (ADC 27):**
- Function A (depends on selected function)
- When selector = 7: Tempo (40-360 BPM)

**Functions:**
0. Sample selection
1. Filter cutoff
2. Gate length
3. Jump probability
4. Tunnel probability
5. Sequencer recording enable
6. Save settings
7. Tempo

### Rotary Encoder

- **Rotate:** Adjust BPM (40-360 BPM range)
- **Button Press:** (reserved for future use)

### Buttons

**Button A (GPIO 7):**
- Mute/Unmute playback

**Button B (GPIO 8):**
- (reserved for future use)

### OLED Display (72x40)

The display shows:
- **S:** Current sample number
- **B:** Current beat position
- **BPM:** Current tempo
- **MUTED:** Mute status

Auto-sleep after 3 minutes of inactivity to prevent burn-in.

## MIDI Operation

### DIN MIDI Input

SumaPiko responds to DIN MIDI on GPIO 1 (31250 baud):

**Note On (36-43):** Triggers beats 0-7
- C2 (36) = Beat 0
- D2 (38) = Beat 1
- ...
- C3 (48) = Beat 7

**Note Off:** Releases the beat

**Start:** Begin playback
**Continue:** Resume playback
**Stop:** Mute playback
**Clock:** Sync to external MIDI clock

**Note:** USB MIDI is not supported (UART DIN MIDI only).

## Features

### Probability Mutations

- **Jump:** Randomly jump to different beats
- **Gate:** Randomly silence beats
- **Direction:** Randomly reverse playback
- **Tunnel:** Randomly switch to different samples

### Retriggering

16 different retrigger patterns create rhythmic variations:
- Short retriggers (fast)
- Long retriggers (slow)
- Pitch shifts (up/down)
- Filter sweeps
- Volume envelopes

### Effects

- **Distortion:** Wave-folding and clipping
- **Low-pass Filter:** Pre-computed biquad filters
- **Noise Gate:** Removes low-level noise
- **Crossfading:** Smooth transitions between beats

## DSP Specifications

- **Sample Rate:** 31kHz (lo-fi character)
- **Sample Format:** 8-bit unsigned (0-255, 128 = silence)
- **Output:** 16-bit stereo I2S (PT8211 DAC)
- **Architecture:** Dual-core (Core 0: UI/MIDI, Core 1: Audio)

## Audio Storage

Samples are embedded in flash memory using the `__in_flash()` attribute. The `audio2h.h` file contains:

- Sample data (8-bit unsigned)
- Beat markers
- Length information
- Retrigger patterns

**Default Sample:** Amen break (170 BPM, 16 beats)

## Troubleshooting

### No Audio Output

1. Check I2S wiring (GPIO 19, 20)
2. Verify PT8211 DAC power
3. Check sample rate (31kHz)
4. Verify not muted (Button A)

### No MIDI Response

1. Verify DIN MIDI connected to GPIO 1
2. Check optoisolator wiring (6N137 required)
3. Verify MIDI device sends on any channel
4. Test with MIDITester application

### OLED Not Displaying

1. Check I2C wiring (GPIO 4, 5)
2. Verify OLED address (0x3C)
3. Check 3.3V power supply

### Keys Not Responding

1. Verify key matrix wiring (GPIO 12-17)
2. Check Cherry MX switches are seated properly
3. Test with multimeter in diode mode

## Known Limitations

- **No USB MIDI:** Only DIN MIDI input supported
- **Fixed Sample Rate:** 31kHz (cannot be changed at runtime)
- **Mono Audio:** Stereo output is duplicated mono
- **No Sample Recording:** Samples must be pre-converted

## License

**SumaPiko modifications:** MIT License - See LICENSE in parent directory.

**Original pikocore:** MIT License - See https://github.com/schollz/pikocore

**Hardware design:** Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)

## Credits

**Original pikocore:** [schollz](https://github.com/schollz)

**SumaPiko adaptation:** [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)

**SumaShield hardware:** [Hugelton instruments](https://github.com/hugelton)

## See Also

- [pikocore](https://github.com/schollz/pikocore) - Original lo-fi music mangler
- [SumaShield](https://github.com/hugelton/SumaShield) - Hardware platform
- [SumaDrums](../SumaDrums/) - 8-track drum sequencer
- [SumaFormants](../SumaFormants/) - Formant vocoder synth
