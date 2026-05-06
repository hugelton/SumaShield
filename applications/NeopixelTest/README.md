# NeopixelTest

**Test program for SumaShield NeoPixel (WS2812) on GPIO 6**

**Version:** 1.0.0

## Overview

NeopixelTest is a diagnostic tool for testing the NeoPixel RGB LED on SumaShield hardware. It uses the FastLED library to drive the WS2812 LED connected to GPIO 6.

## Hardware

**SumaShield Platform v0.5 or later:**
- Raspberry Pi Pico / Pico 2 / Clone boards
- **NeoPin:** GPIO 6 (WS2812 RGB LED)
- **Buttons:** GPIO 7 (A), GPIO 8 (B)

## Features

### Test Patterns (8 patterns)

1. **Rainbow** - Cycle through all colors smoothly
2. **Blink** - Blink white on/off (500ms)
3. **Pulse** - Red pulse with breathing effect
4. **Red** - Solid red
5. **Green** - Solid green
6. **Blue** - Solid blue
7. **White** - Solid white
8. **Test All** - Rapid color cycling (R, G, B, Y, C, M, W, Off)

### Controls

**Button A (GPIO 7):**
- **Short Press:** Cycle through 8 test patterns
- Flash white 3x to confirm pattern change

**Button B (GPIO 8):**
- **Short Press:** Cycle brightness levels
- Levels: 32 → 64 → 128 → 192 → 255 (loops back to 32)
- Flash white once to confirm brightness change

## Quick Start

### Prerequisites

1. Install Arduino IDE 2.x
2. Add Raspberry Pi Pico board support
3. Install **FastLED** library:
   - Tools → Manage Libraries...
   - Search for "FastLED"
   - Install "FastLED" by Daniel Garcia
4. Set USB Stack to "TinyUSB"

### Upload

1. Open `NeopixelTest.ino` in Arduino IDE
2. Select board: "Raspberry Pi Pico"
3. Set USB Stack: "TinyUSB"
4. Hold BOOTSEL button, plug Pico into USB
5. Click Upload

### Initial Test Sequence

On startup, the program will:
1. Show **Red** for 200ms
2. Show **Green** for 200ms
3. Show **Blue** for 200ms
4. Turn **Off** LED
5. Begin **Rainbow** pattern (default)

## Serial Monitor Output

Open Serial Monitor (115200 baud) to see:

```
========================================
   NeopixelTest for SumaShield
========================================

NeoPin: GPIO 6
LED Type: WS2812B
Brightness: 64

Controls:
  Button A: Next pattern
  Button B: Increase/decrease brightness

Patterns:
  0: Rainbow
  1: Blink
  2: Pulse
  3: Red
  4: Green
  5: Blue
  6: White
  7: Test All

========================================
Initializing...

Test sequence: R -> G -> B

Ready! Current pattern: Rainbow
Press Button A to change patterns
Press Button B to adjust brightness
========================================
```

## Pattern Descriptions

### Rainbow (Default)
- Smoothly cycles through all colors (HSV color wheel)
- Fast update rate (20ms per step)
- Good for testing color accuracy and responsiveness

### Blink
- Blinks white on/off
- 500ms interval
- Good for testing basic on/off functionality

### Pulse
- Red color pulses smoothly
- Breathing effect (fade in/out)
- Good for testing brightness control

### Solid Colors (Red, Green, Blue, White)
- Constant color output
- Good for:
  - Verifying specific colors work
  - Checking color accuracy
  - Testing stability under constant load

### Test All
- Rapid color cycling: R → G → B → Y → C → M → W → Off
- 200ms per color
- Good for:
  - Full color range test
  - Checking all primary and secondary colors
  - Verifying LED can turn off completely

## Troubleshooting

### LED Not Lighting Up

**Possible causes:**
1. **Wrong GPIO pin:** Verify solder joint to GPIO 6
2. **No power:** Check 3.3V supply to LED
3. **Data line issue:** Check solder joint on GPIO 6
4. **LED defective:** Try replacing NeoPixel
5. **Code not uploaded:** Verify "Done uploading" message

**Testing steps:**
1. Check Serial Monitor output
2. Verify GPIO 6 is not used by other components
3. Test with multimeter:
   - Measure voltage on GPIO 6 (should pulse when sending data)
   - Verify 3.3V supply to LED VCC

### Wrong Colors

**Possible causes:**
1. **Color order mismatch:** Try changing `COLOR_ORDER`:
   ```cpp
   #define COLOR_ORDER RGB   // Red-Green-Blue
   #define COLOR_ORDER GRB   // Green-Red-Blue (most common)
   #define COLOR_ORDER BRG   // Blue-Red-Green
   ```
2. **LED type mismatch:** Try changing `LED_TYPE`:
   ```cpp
   #define LED_TYPE WS2812    // Original WS2812
   #define LED_TYPE WS2812B   // Improved version (most common)
   #define LED_TYPE SK6812    // Different LED type
   ```

### Flickering or Unstable Behavior

**Possible causes:**
1. **Insufficient power:** WS2812 can draw up to 60mA at full white brightness
2. **Ground connection:** Verify solid ground connection
3. **Data signal quality:** Try adding 300-500Ω resistor in series with data line
4. **Power supply noise:** Add 100µF capacitor across LED power and ground

**Solutions:**
- Reduce brightness (start at 64 or lower)
- Check power supply capacity (needs at least 100mA headroom)
- Verify good ground connection
- Add decoupling capacitor (10µF or larger)

### LED Too Bright

**Solutions:**
1. Reduce `BRIGHTNESS` constant:
   ```cpp
   #define BRIGHTNESS 32  // Start low (0-255)
   ```
2. Use Button B to cycle through brightness levels
3. For indoor use, 64-128 is usually sufficient

## FastLED Configuration

### Current Settings

```cpp
#define NEOPIXEL_PIN     6     // GPIO 6 on SumaShield
#define NUM_LEDS         1     // Single LED
#define LED_TYPE         WS2812B
#define COLOR_ORDER      GRB   // Most WS2812B are GRB
#define BRIGHTNESS       64    // Start at 25% brightness
```

### Adjusting for Different LED Types

**For WS2812 (original):**
```cpp
#define LED_TYPE    WS2812
#define COLOR_ORDER RGB
```

**For SK6812:**
```cpp
#define LED_TYPE    SK6812
#define COLOR_ORDER RGB
```

**For PL9823:**
```cpp
#define LED_TYPE    PL9823
#define COLOR_ORDER RGB
```

## Advanced Usage

### Custom Patterns

To add custom patterns, modify the `runPattern()` function:

```cpp
case CUSTOM_PATTERN:
  // Your custom pattern code here
  leds[0] = CRGB::Purple;
  if (current_time - last_update > 1000) {
    // Update every second
    leds[0].fadeToBlackBy(10);
    last_update = current_time;
  }
  break;
```

### HSV Colors

FastLED supports HSV color space:

```cpp
// HSV (Hue, Saturation, Value)
// Hue: 0-255 (color wheel)
// Saturation: 0-255 (0=gray, 255=vivid)
// Value: 0-255 (0=black, 255=bright)

leds[0] = CHSV(128, 255, 255);  // Cyan (Hue 128)
```

### Color Mixing

```cpp
// Add colors
leds[0] = CRGB::Red + CRGB::Blue;  // Magenta

// Fade to black
leds[0].fadeToBlackBy(128);  // 50% brightness

// Scale brightness
leds[0] = CRGB::Green;
leds[0].nscale8(128);  // 50% brightness
```

## Technical Specifications

**WS2812B LED:**
- **Supply Voltage:** 3.5-5.3V (works with 3.3V Pico)
- **Current per LED:** ~20mA (red), ~20mA (green), ~20mA (blue)
- **Peak Current (white):** ~60mA
- **Data Protocol:** 800kHz PWM-like signal
- **Color Depth:** 24-bit (8-bit per channel)
- **Refresh Rate:** Can be updated at ~400Hz maximum

**FastLED Library:**
- Efficient bit-banging for RP2040
- Supports various LED types
- HSV color space support
- Built-in effects

## Safety Notes

1. **Current Limit:** WS2812B can draw up to 60mA at full white
   - RP2040 GPIO max: 16mA per pin
   - **Solution:** LED powers itself from VCC, not GPIO
   - GPIO only provides data signal (low current)

2. **3.3V Compatibility:**
   - WS2812B works at 3.3V (colors may be slightly dimmer than 5V)
   - Data signal from Pico (3.3V logic) is sufficient
   - No level shifter required

3. **Eye Safety:**
   - Avoid staring at full-brightness white LED for extended periods
   - Reduce brightness for close-up viewing

## References

- [FastLED Library](https://github.com/FastLED/FastLED)
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [SumaShield Hardware](../../HARDWARE.md)
- [Neopixel Guide](https://learn.adafruit.com/adafruit-neopixel-uberguide)

## Author

**NeopixelTest** by [Leo Kuroshita](https://github.com/kurogedelic) · [@kurogedelic](https://x.com/kurogedelic)

**SumaShield** by [Hugelton instruments](https://github.com/hugelton)
