# PixelSynth

**16x16 LED Matrix Visual Synth for SumaShield**

**Version:** 1.0.0

## Overview

PixelSynth is a visual synthesizer that generates real-time animated patterns on a 16x16 LED matrix. Inspired by shader art and demoscene aesthetics, it creates organic, flowing visuals using procedural noise and mathematical functions.

**⚠️ HARDWARE NOTE:** NeoPixel output is on GPIO 2. This requires external LED matrix hardware connected to GPIO 2.

## Features

- **6 Visual Modes**: Noise Mesh, Voronoi, Plasma, Ripples, Matrix Rain, Fire
- **Real-time Parameter Control**: Adjust speed, scale, color, and effects on the fly
- **VJ Flash Effect**: Instant white flash for visual performances
- **OLED Display**: Shows current mode and parameter values
- **Key-based Mode Switching**: Press keys 0-5 to switch visual modes
- **Rotary Fine Adjustment**: Subtle parameter tweaks via rotary encoder

## Hardware Requirements

**SumaShield Platform v0.5 or later:**
- Raspberry Pi Pico / Pico 2
- **External LED matrix** connected to GPIO 2 (WS2812B/SK6812, 16x16 = 256 LEDs)
- OLED SSD1306 (72x40)
- Key Matrix (4x2 = 8 keys)
- Rotary encoder
- 2x Potentiometers
- Button A

**⚠️ External Hardware Required:** You must connect your own 16x16 LED matrix to GPIO 2. This is not included on the SumaShield PCB.

## Controls

### Key Matrix (Keys 0-7)

| Key | Mode | Description |
|-----|------|-------------|
| 0 | Noise Mesh | Woven noise patterns with organic movement |
| 1 | Voronoi | Cellular patterns with moving points |
| 2 | Plasma | Fractal-like flowing color gradients |
| 3 | Ripples | Expanding concentric rings from center |
| 4 | Matrix Rain | Digital rain effect (Matrix-style) |
| 5 | Fire | Rising flames with color gradients |
| 6 | Tetris X | 2x2 block, move left/right with Knob 1 |
| 7 | Tetris Rot | 4x1 block, rotate with Button A |

### Knob 1 (ADC 26) - Speed / Scale

- **Effect**: Controls animation speed and pattern scale
- **Range**: 1-30 (fast) to 100 (slow, large patterns)
- **Per-mode behavior**:
  - Noise Mesh, Voronoi, Plasma: Pattern scale
  - Ripples, Matrix Rain, Fire: Animation speed

### Knob 2 (ADC 27) - Color / Brightness

- **Effect**: Controls color hue offset
- **Range**: 0-255 (full color spectrum)
- **Note**: Brightness is fixed at 100 (set in code)

### Rotary Encoder - Fine Adjustment

- **Effect**: Subtle parameter offset
- **Range**: -50 to +50
- **Per-mode behavior**:
  - Noise Mesh, Voronoi, Plasma: Position offset
  - Ripples: Phase shift
  - Matrix Rain, Fire: Turbulence adjustment

### Button A (GPIO 7) - Flash Effect

- **Press**: Fill entire matrix with white (VJ strob effect)
- **Release**: Return to current mode
- **Use**: Visual performances, transitions, rhythm accents

## OLED Display (72x40)

**Top Row:**
- `M:` Current mode number and name
- `FLASH` indicator (when Button A pressed)

**Middle Rows:**
- `K1:` Knob 1 value (0-100)
- `K2:` Knob 2 value (0-255)
- `ROT:` Rotary offset (-50 to +50)

**Auto-sleep:** OLED turns off after 3 minutes of inactivity

## Quick Start

### Prerequisites

1. Install Arduino IDE 2.x
2. Add Raspberry Pi Pico board support
3. Install libraries:
   - **FastLED** by Daniel Garcia
   - Wire (built-in)
   - U8g2 (OLED display)
4. Set USB Stack to "TinyUSB"

### Upload

1. Open `PixelSynth.ino` in Arduino IDE
2. Select board: "Raspberry Pi Pico"
3. Set USB Stack: "TinyUSB"
4. Hold BOOTSEL button, plug Pico into USB
5. Click Upload

### First Run

```
PixelSynth v1.0.0
16x16 LED Matrix Visual Synth for SumaShield

PixelSynth initialized
NeoPixel on GPIO 2
```

**OLED Display:**
```
M:0 Noise Mesh
K1:50 K2:128
ROT:+0
```

## Visual Modes

### Mode 0: Noise Mesh

Organic, woven patterns using Perlin noise. Creates flowing, wave-like structures that evolve over time.

**Parameters:**
- Knob 1: Noise scale (zoom level)
- Knob 2: Color offset (cycle through colors)
- Rotary: X/Y position offset

### Mode 1: Voronoi

Cellular patterns based on distance to moving points. Creates biological, cell-like structures.

**Parameters:**
- Knob 1: Cell size scale
- Knob 2: Color gradient
- Rotary: Point movement offset

### Mode 2: Plasma

Fractal-like flowing color gradients using sine waves. Creates smooth, liquid patterns.

**Parameters:**
- Knob 1: Plasma frequency (detail level)
- Knob 2: Color offset
- Rotary: Phase shift

### Mode 3: Ripples

Expanding concentric rings from center. Creates underwater ripple effects.

**Parameters:**
- Knob 1: Ripple frequency (how close together)
- Knob 2: Base color
- Rotary: Phase shift (ripple animation offset)

### Mode 4: Matrix Rain

Digital rain effect inspired by The Matrix. Drops fall randomly in columns.

**Parameters:**
- Knob 1: Fall speed and density
- Knob 2: Green color intensity
- Rotary: Turbulence (randomness)

### Mode 5: Fire

Rising flames with color gradients (red → orange → yellow). Simulates fire or aurora.

**Parameters:**
- Knob 1: Flame height and turbulence
- Knob 2: Color temperature (red → yellow)
- Rotary: Wind/drift offset

### Mode 6: Tetris X

Interactive 2x2 tetromino block that moves horizontally. Simple demo of game mechanics.

**Parameters:**
- Knob 1: Horizontal position (left/right)
- Rotary: Fine position adjustment
- Button A: No effect (use flash)

### Mode 7: Tetris Rot

Interactive 4x1 tetromino that rotates 90 degrees. Demonstrates piece rotation mechanics.

**Parameters:**
- Knob 1: No effect
- Rotary: Fine position adjustment
- Button A: Rotate 90 degrees (cycles 0→1→2→3→0)

## Performance Tips

### Frame Rate

- Target: ~100 FPS (10ms loop delay)
- FastLED uses DMA, so CPU is mostly free
- OLED updates at ~30 FPS (33ms) to reduce flicker

### Color Order

- WS2812B typically uses **GRB** color order
- If colors look wrong (red ↔ green), change `COLOR_ORDER`:
  ```cpp
  #define COLOR_ORDER RGB   // Try this if colors are swapped
  #define COLOR_ORDER GRB   // Default for WS2812B
  ```

### Brightness

- Default: 100 (out of 255)
- To reduce power consumption or eye strain:
  ```cpp
  #define BRIGHTNESS 50  // Half brightness
  ```

### Power Consumption

- 256 LEDs at full white: ~15A (not possible via USB!)
- Typical patterns: ~2-3A (color-heavy, dim pixels)
- Recommended: Use powered USB hub or external 5V supply

## Troubleshooting

### LEDs Not Lighting Up

1. Check wiring from GPIO 2 to LED matrix data input
2. Verify 5V power supply (needs 2-3A for full brightness)
2. Verify 5V power supply (needs 2-3A for full brightness)
3. Check ground connection
4. Try reducing `BRIGHTNESS` to 50

### Wrong Colors

1. Check `COLOR_ORDER` definition
2. Try swapping `RGB` ↔ `GRB`
3. Verify LED type is WS2812B (not SK6812)

### Flickering OLED

1. Reduce OLED update rate (change 33ms to 50ms)
2. Check I2C wiring (GPIO 4/5)
3. Verify 3.3V power supply

### No Response to Controls

1. Verify key matrix wiring (GPIO 12-17)
2. Check rotary encoder pins (GPIO 9-11)
3. Test knobs with multimeter (ADC 26/27)
4. Check Serial Monitor output

## Technical Specifications

**LED Matrix:**
- Type: WS2812B / SK6812MINI-E (user-provided)
- Size: 16x16 = 256 pixels
- Data Pin: GPIO 2 (external connection)
- Color Order: GRB (configurable)
- Max Current: ~15A (theoretical, full white)
- Typical Current: ~2-3A (colorful patterns)

**Display:**
- Resolution: 72x40 pixels
- Controller: SSD1306
- Interface: I2C (GPIO 4/5)
- Refresh Rate: ~30fps

**Performance:**
- Frame Rate: ~100 FPS (LED matrix)
- CPU Load: ~30% (RP2040, single-core)
- Memory: ~3KB (LED buffer + variables)

## Known Limitations

- **External LED required**: 16x16 LED matrix must be connected to GPIO 2
- **No USB MIDI**: Not implemented (visual-only synth)
- **Single-core**: Uses Core 0 only (no audio output)
- **Fixed matrix size**: Hardcoded for 16x16 (256 LEDs)

## Future Enhancements

Possible additions for future versions:

- Audio reactivity (microphone input on ADC3)
- MIDI sync (beat-synchronized patterns)
- Recording/playback of parameter sequences
- Additional visual modes
- Adjustable brightness via encoder

## License

**Firmware:** MIT License - See [LICENSE](../../LICENSE) for details.

**Hardware:** Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0).

See [HARDWARE.md](../../HARDWARE.md) and [LICENSES/CC-BY-SA-4.0.txt](../../LICENSES/CC-BY-SA-4.0.txt) for details.

## Credits

**Original Concept:** Pixel Synth visualizer

**SumaShield Adaptation:** [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)

**SumaShield Hardware:** [Hugelton instruments](https://github.com/hugelton)

## References

- [FastLED Library](https://github.com/FastLED/FastLED)
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [SK6812MINI-E Datasheet](https://cdn-shop.adafruit.com/product-files/4382/SK6812MINI-E+20200511.pdf)
- [SumaShield Hardware](../../HARDWARE.md)
