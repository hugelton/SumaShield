# SumaDrums

**8-track drum sequencer with step recording and dual MIDI input**

**Version:** 1.0.0

## Overview

SumaDrums is an 8-track drum sequencer with real-time parameter control, running on Raspberry Pi Pico RP2040.

## Features

- **8 Tracks**: Kick, Snare, Closed Hat, Open Hat, Tom1, Tom2, Clap, Cymbal
- **Step Recording**: Record patterns in real-time
- **Dual MIDI**: USB MIDI + DIN MIDI input
- **Real-time Parameters**: Decay and Pitch control per track
- **Preset Patterns**: 8 built-in drum patterns
- **Sequencer**: 16-step patterns with 8 tracks
- **Auto-sleep**: OLED turns off after 3 minutes to prevent burn-in

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

1. Open `SumaDrums.ino` in Arduino IDE
2. Select board: "Raspberry Pi Pico"
3. Set USB Stack: "TinyUSB"
4. Hold BOOTSEL button, plug Pico into USB
5. Click Upload

## Controls

### Button A: Play/Stop

- **Short Press (<1s):** Toggle sequencer play/stop
- **Long Press (>1s):** No function (reserved)

### Button B: Record Mode

- **Short Press (<1s):** Toggle record mode
- **Long Press (>1s):** Delete mode (press keys to delete notes at current step)

### Rotary Encoder

- **Rotation:** Adjust BPM (40-300 BPM range)
- **Button Press:** No function (reserved)

### Potentiometers

- **ADC 26 (Decay):** Adjust decay time for last edited track
- **ADC 27 (Pitch):** Adjust pitch for last edited track

**Smart Parameter Control:**
- Pots remember which track they last edited
- Decay and Pitch pots are independent
- Trigger a track, then adjust pot to edit that track

### Key Matrix (8 Keys)

Physical keys correspond to tracks 0-7:
- **Track 0:** Kick (C4, MIDI note 60)
- **Track 1:** Snare (D4, MIDI note 62)
- **Track 2:** Closed Hat (E4, MIDI note 64)
- **Track 3:** Open Hat (F4, MIDI note 65)
- **Track 4:** Tom1 (G4, MIDI note 67)
- **Track 5:** Tom2 (A4, MIDI note 69)
- **Track 6:** Clap (B4, MIDI note 71)
- **Track 7:** Cymbal (C5, MIDI note 72)

## Operating Modes

### Play Mode (Default)

- Sequencer auto-plays 16-step pattern
- Keys trigger manual preview (don't change pattern)
- Adjust BPM with rotary encoder
- Edit track parameters with pots

### Record Mode

- Keys toggle steps at current position
- Metronome click on every beat (strong on beat 1)
- Real-time pattern editing
- Immediate audio feedback

### Delete Mode

- Hold Button B (>1s) to enter Delete mode
- Press keys to delete notes at current step
- Release Button B to exit Delete mode

## MIDI Operation

### USB MIDI

1. Connect USB MIDI controller to computer
2. Select SumaDrums as MIDI output in DAW/sequencer
3. Play notes C4-C5 to trigger drums
4. MIDI notes respect Play/Record mode setting

### DIN MIDI

1. Connect MIDI device to GPIO 1 (via optoisolator)
2. Play notes C4-C5 to trigger drums
3. Simultaneous use with USB MIDI

**MIDI Features:**
- Note On: Triggers drum sound
- Note Off: No function (drums are one-shot)
- Velocity: No function (fixed volume)
- Channel: Omni (responds to all channels)

## Preset Patterns

SumaDrums includes 8 built-in patterns:
- Pattern 0: Basic Groove
- Pattern 1: House (4-on-the-floor)
- Pattern 2: Breakbeat
- Pattern 3: Electro
- Pattern 4: Trap Style
- Pattern 5: Dembow / Reggaeton
- Pattern 6: Minimal
- Pattern 7: Busy Fill

**Note:** Presets are loaded into tracks 0-3 only. Tracks 4-7 start empty for user programming.

## Specifications

**Audio:**
- Sample Rate: 22.05kHz
- Bit Depth: 16-bit
- Format: LSBJ (Least-Significant-Bit-Justified)
- Output: Mono (duplicated to L+R)

**Display:**
- Resolution: 72x40 pixels
- Controller: SSD1306
- Interface: I2C (GPIO 4/5)
- Auto-sleep: 3 minutes

**Performance:**
- CPU Usage: Dual-core (Core 0: UI, Core 1: Audio)
- Polyphony: 8 voices
- Sequencer: 8 tracks × 16 steps

## OLED Display (72x40)

**Display Shows:**
- **Top Row:** Track number and instrument name
- **Middle:** Decay (left) and Pitch (right) values
- **Bottom:** BPM, mode (Play/Record), and version

**Example Display:**
```
T2 CH
0.85 1.23
D<   P
BPM:120 R
```

**Meaning:**
- Track 2: Closed Hat
- Decay: 0.85 (being edited, marked with "<")
- Pitch: 1.23
- BPM: 120
- Mode: Record (R)
- Version: 1.0.0 (shown on startup and occasionally)

## Troubleshooting

### No Sound Output

1. Check I2S wiring (GPIO 19, 20)
2. Verify PT8211 DAC power supply
3. Check sample rate (22.05kHz)
4. Verify not muted (Button A)

### No MIDI Response

1. Verify "TinyUSB" stack selected in Arduino IDE
2. Check DIN MIDI wiring (GPIO 1)
3. Test with MIDI monitoring software
4. Verify optoisolator connections

### OLED Not Displaying

1. Check I2C wiring (GPIO 4, 5)
2. Verify OLED address (0x3C)
3. Check 3.3V power supply

### Keys Not Responding

1. Verify key matrix wiring (GPIO 12-17)
2. Check Cherry MX switches are seated properly
3. Test with multimeter in diode mode

## Version History

### Version 1.0.0 (2026-05-06)
- Initial release
- 8-track drum sequencer
- Dual MIDI input (USB + DIN)
- 8 preset patterns
- Real-time parameter control
- OLED display with auto-sleep

## Author

**SumaDrums** by [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)

**SumaShield** by [Hugelton instruments](https://github.com/hugelton)

## License

**Firmware:** MIT License

**Hardware:** Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)

See [LICENSE](../../LICENSE) and [HARDWARE.md](../../HARDWARE.md) for details.
