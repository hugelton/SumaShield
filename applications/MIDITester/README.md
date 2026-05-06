# MIDITester

**MIDI diagnostic tool for debugging reception issues**

**Version:** 1.0.0

## Overview

MIDITester is a diagnostic tool for debugging MIDI reception issues on SumaShield hardware. It monitors both USB MIDI and DIN MIDI simultaneously, detecting dropped messages, analyzing timing intervals, and verifying MIDI controller functionality.

## Features

- **Dual MIDI Monitoring:** USB + DIN MIDI simultaneously
- **Message Counting:** Separate counters for USB and Serial MIDI
- **Interval Tracking:** Measures time between MIDI messages
- **Detailed Logging:** Serial output shows message type, note, velocity
- **LED Feedback:** Visual indication for USB (short flash) and Serial (long flash)
- **OLED Display:** Real-time stats display (72x40)
- **Auto-sleep:** OLED turns off after 3 minutes

## Hardware Requirements

**SumaShield Platform v0.5 or later** (shield for Raspberry Pi Pico/Pico 2/Clones):
- Raspberry Pi Pico / Pico 2 / Clone boards
- **On-board I2S DAC:** PT8211 (integrated on shield)
- OLED SSD1306 (72x40)
- **On-board LED:** GPIO 25 (Pico LED)
- **DIN MIDI Input** (via optoisolator, GPIO 1)

## Pin Configuration

```
I2C Display:  GPIO 4 (SDA), GPIO 5 (SCL)
LED:          GPIO 25 (Pico on-board LED)
MIDI DIN:     GPIO 1 (RX) - 31250 baud
```

## Quick Start

### Prerequisites

1. Install Arduino IDE 2.x
2. Add Raspberry Pi Pico board support
3. Install libraries:
   - Wire (built-in)
   - U8g2 (OLED display)
   - Adafruit TinyUSB Library
   - MIDI Library by Forty Seven Effects
4. Set USB Stack to "TinyUSB"

### Upload

1. Open `MIDITester.ino` in Arduino IDE
2. Select board: "Raspberry Pi Pico"
3. Set USB Stack: "TinyUSB"
4. Hold BOOTSEL button, plug Pico into USB
5. Click Upload

## Display (72x40 OLED)

**Top Row:**
- `U:` USB message count
- `S:` Serial message count

**Middle Rows:**
- Last USB MIDI message (type + data)
- Last Serial MIDI message (type + data)

**Bottom Row:**
- USB interval (ms since last USB message)
- Serial interval (ms since last Serial message)

## Serial Output (115200 baud)

**Format:**
```
[USB] #42 Ch:1 NoteOn 60 127 (15ms)
[SERIAL] #103 Ch:1 NoteOff 60 0 (20ms)
```

**Meaning:**
- `#42`: Message number
- `Ch:1`: MIDI channel
- `NoteOn`: Message type
- `60`: Note number (C4)
- `127`: Velocity
- `(15ms)`: Time since last message

## LED Feedback

**USB MIDI:**
- Short flash (1ms)

**Serial MIDI:**
- Longer flash (2ms)

## Usage

### Test 1: USB MIDI Continuity

1. Connect USB MIDI controller
2. Send continuous messages (arpeggio, sequence)
3. Check interval times in Serial output
4. Should be consistent (e.g., 10-20ms for 100 BPM)
5. Gaps indicate dropped messages

### Test 2: Serial DIN MIDI Accuracy

1. Connect DIN MIDI device to GPIO 1
2. Compare message count with device output
3. Check for missing Note Off messages
4. Verify velocity values

### Test 3: Dual Input Stress

1. Send USB and Serial MIDI simultaneously
2. Verify both counters increment
3. No cross-talk between inputs
4. LED flashes appropriately

## Diagnostics

### Symptom: Intermittent MIDI reception

**USB MIDI:**
- Check interval times (look for large gaps)
- Verify USB cable (data cable, not charge-only)
- Test with different MIDI controller

**DIN MIDI:**
- Check optoisolator wiring (6N137)
- Verify baud rate (31250)
- Test with known-good MIDI device

### Symptom: Missing Note Off messages

- Check MIDI device running status
- Verify MIDI implementation in device
- Use MIDITester to capture raw data

### Symptom: Wrong note numbers

- Verify MIDI note mapping
- Check MIDI channel (omni mode)
- Test with known-good MIDI monitor

## Specifications

**Display:**
- Resolution: 72x40 pixels
- Controller: SSD1306
- Interface: I2C (GPIO 4/5)
- Refresh rate: ~30fps (33ms)

**MIDI:**
- USB MIDI: TinyUSB stack
- DIN MIDI: 31250 baud UART (GPIO 1)
- Channels: Omni (all channels)

**Performance:**
- CPU: Single-core (no DSP required)
- Memory: Minimal footprint

## Troubleshooting

### OLED Not Displaying

1. Check I2C wiring (GPIO 4, 5)
2. Verify OLED address (0x3C)
3. Check 3.3V power supply
4. Test with I2C scanner sketch

### No LED Feedback

1. Verify GPIO 25 connection
2. Check LED solder joint
3. Test LED with simple blink sketch

### No MIDI Detected

1. **CRITICAL:** Verify USB Stack is "TinyUSB"
2. Restart Arduino IDE after changing USB Stack
3. Replug Pico USB cable
4. Test with MIDI monitoring software

### Serial Monitor Garbage

1. Check baud rate is 115200
2. Verify line ending is Both NL & CR
3. Reopen Serial Monitor after upload

## Version History

### Version 1.0.0 (2026-05-06)
- Initial release
- USB + DIN MIDI simultaneous monitoring
- Message counting and interval tracking
- OLED display with real-time stats
- LED feedback for visual debugging
- Serial logging with detailed message info

## Use Cases

### 1. Verify MIDI Controller Functionality

Test your MIDI controller to ensure it's sending messages correctly:
- Play notes and check message count
- Verify note numbers are correct
- Check velocity values

### 2. Debug MIDI Reception Issues

When SumaDrums or SumaFormants isn't responding to MIDI:
- Use MIDITester to verify MIDI is received
- Check message types and timing
- Identify if specific messages are dropped

### 3. Test Optoisolator Circuit

For DIN MIDI input troubleshooting:
- Connect DIN MIDI device
- Verify Serial message count increments
- Check for missing or corrupted messages

### 4. Analyze MIDI Timing

For timing-sensitive applications:
- Check interval times between messages
- Verify clock stability
- Identify jitter or latency issues

## Example Output

**Serial Monitor:**
```
========================================
MIDITester - USB + Serial MIDI Monitor
Version 1.0.0
========================================
Monitoring both USB and DIN MIDI...
Connect your MIDI controller and play!

[USB] #1 Ch:1 NoteOn 60 127 (0ms)
[USB] #2 Ch:1 NoteOff 60 0 (120ms)
[SERIAL] #1 Ch:1 NoteOn 64 100 (50ms)
[SERIAL] #2 Ch:1 NoteOff 64 0 (180ms)
```

**OLED Display:**
```
U:0005 S:0002
NoteOn 60
NoteOff 64
15ms   20ms
```

## Author

**MIDITester** by [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)

**SumaShield** by [Hugelton instruments](https://github.com/hugelton)

## License

**Firmware:** MIT License

**Hardware:** Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)

See [LICENSE](../../LICENSE) and [HARDWARE.md](../../HARDWARE.md) for details.
