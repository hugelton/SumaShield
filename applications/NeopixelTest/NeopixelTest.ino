/*
 * NeopixelTest
 *
 * Test program for SumaShield NeoPixel (WS2812) on GPIO 6
 * Uses FastLED library for LED control
 *
 * Author: Leo Kuroshita (Hugelton instruments)
 */

#include <FastLED.h>

// Version information
#define NEOPIXELTEST_VERSION_MAJOR 1
#define NEOPIXELTEST_VERSION_MINOR 0
#define NEOPIXELTEST_VERSION_PATCH 0
#define NEOPIXELTEST_VERSION "1.0.0"

// NeoPixel configuration
#define NEOPIXEL_PIN     6     // GPIO 6
#define NUM_LEDS         1     // Single NeoPixel
#define LED_TYPE         WS2812B
#define COLOR_ORDER      GRB
#define BRIGHTNESS       64    // 0-255 (start low, adjust as needed)

// CRGB array for FastLED
CRGB leds[NUM_LEDS];

// Animation timing
unsigned long last_update = 0;
unsigned long update_interval = 500; // ms between color changes
uint8_t hue = 0;  // Color wheel position (0-255)
uint8_t pattern = 0;  // Current pattern (0-7)

// Button for pattern switching
const int PIN_BTN_A = 7;
const int PIN_BTN_B = 8;

bool last_btn_a = HIGH;
bool last_btn_b = HIGH;

// Pattern definitions
enum Pattern {
  RAINBOW = 0,        // Cycle through colors
  BLINK,              // Blink on/off
  PULSE,              // Pulse brightness
  RED,                // Solid red
  GREEN,              // Solid green
  BLUE,               // Solid blue
  WHITE,              // Solid white
  TEST_ALL            // Test all colors rapidly
};

const char* PATTERN_NAMES[] = {
  "Rainbow",
  "Blink",
  "Pulse",
  "Red",
  "Green",
  "Blue",
  "White",
  "Test All"
};

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for serial to stabilize

  Serial.println("========================================");
  Serial.println("   NeopixelTest for SumaShield");
  Serial.print("   Version ");
  Serial.println(NEOPIXELTEST_VERSION);
  Serial.println("========================================");
  Serial.println();
  Serial.print("NeoPin: GPIO ");
  Serial.println(NEOPIXEL_PIN);
  Serial.print("LED Type: WS2812B");
  Serial.println();
  Serial.print("Brightness: ");
  Serial.println(BRIGHTNESS);
  Serial.println();
  Serial.println("Controls:");
  Serial.println("  Button A: Next pattern");
  Serial.println("  Button B: Increase/decrease brightness");
  Serial.println();
  Serial.println("Patterns:");
  for (int i = 0; i < 8; i++) {
    Serial.print("  ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(PATTERN_NAMES[i]);
  }
  Serial.println();
  Serial.println("========================================");
  Serial.println("Initializing...");
  Serial.println();

  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, NEOPIXEL_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Initialize buttons
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);

  // Test sequence - show each color briefly
  Serial.println("Test sequence: R -> G -> B");
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(200);

  leds[0] = CRGB::Green;
  FastLED.show();
  delay(200);

  leds[0] = CRGB::Blue;
  FastLED.show();
  delay(200);

  FastLED.clear();
  FastLED.show();

  Serial.println();
  Serial.println("Ready! Current pattern: Rainbow");
  Serial.println("Press Button A to change patterns");
  Serial.println("Press Button B to adjust brightness");
  Serial.println();
}

void loop() {
  unsigned long current_time = millis();

  // Read buttons
  bool btn_a = digitalRead(PIN_BTN_A);
  bool btn_b = digitalRead(PIN_BTN_B);

  // Button A: Next pattern
  if (btn_a == LOW && last_btn_a == HIGH) {
    pattern = (pattern + 1) % 8;
    Serial.print("Pattern changed to: ");
    Serial.print(pattern);
    Serial.print(" - ");
    Serial.println(PATTERN_NAMES[pattern]);

    // Flash to confirm pattern change
    for (int i = 0; i < 3; i++) {
      leds[0] = CRGB::White;
      FastLED.show();
      delay(50);
      FastLED.clear();
      FastLED.show();
      delay(50);
    }

    // Reset timing
    last_update = current_time;
    hue = 0;
  }

  // Button B: Cycle brightness
  if (btn_b == LOW && last_btn_b == HIGH) {
    static uint8_t brightness_levels[] = {32, 64, 128, 192, 255};
    static uint8_t brightness_idx = 1;

    brightness_idx = (brightness_idx + 1) % 5;
    uint8_t new_brightness = brightness_levels[brightness_idx];
    FastLED.setBrightness(new_brightness);

    Serial.print("Brightness: ");
    Serial.println(new_brightness);

    // Flash to confirm brightness change
    leds[0] = CRGB::White;
    FastLED.show();
    delay(100);
    FastLED.clear();
    FastLED.show();
  }

  last_btn_a = btn_a;
  last_btn_b = btn_b;

  // Run current pattern
  runPattern(current_time);

  // Update LEDs
  FastLED.show();
}

void runPattern(unsigned long current_time) {
  switch (pattern) {
    case RAINBOW:
      if (current_time - last_update > 20) {  // Fast color cycle
        hue++;
        leds[0] = CHSV(hue, 255, 255);
        last_update = current_time;
      }
      break;

    case BLINK:
      if (current_time - last_update > 500) {  // 500ms blink
        static bool blink_state = false;
        blink_state = !blink_state;
        if (blink_state) {
          leds[0] = CRGB::White;
        } else {
          leds[0] = CRGB::Black;
        }
        last_update = current_time;
      }
      break;

    case PULSE:
      {
        uint8_t pulse_speed = 30;  // ms per step
        if (current_time - last_update > pulse_speed) {
          static uint8_t pulse_value = 0;
          static int8_t pulse_direction = 5;

          pulse_value += pulse_direction;
          if (pulse_value >= 255 || pulse_value <= 0) {
            pulse_direction = -pulse_direction;
          }

          leds[0] = CHSV(0, 255, pulse_value);  // Red pulse
          last_update = current_time;
        }
      }
      break;

    case RED:
      leds[0] = CRGB::Red;
      break;

    case GREEN:
      leds[0] = CRGB::Green;
      break;

    case BLUE:
      leds[0] = CRGB::Blue;
      break;

    case WHITE:
      leds[0] = CRGB::White;
      break;

    case TEST_ALL:
      if (current_time - last_update > 200) {  // Fast color switching
        static uint8_t test_color = 0;

        switch (test_color % 8) {
          case 0: leds[0] = CRGB::Red; break;
          case 1: leds[0] = CRGB::Green; break;
          case 2: leds[0] = CRGB::Blue; break;
          case 3: leds[0] = CRGB::Yellow; break;
          case 4: leds[0] = CRGB::Cyan; break;
          case 5: leds[0] = CRGB::Magenta; break;
          case 6: leds[0] = CRGB::White; break;
          case 7: leds[0] = CRGB::Black; break;
        }

        test_color++;
        last_update = current_time;
      }
      break;
  }
}
