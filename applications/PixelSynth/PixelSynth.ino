/*
 * PixelSynth
 *
 * 16x16 LED Matrix Visual Synth for SumaShield
 *
 * Version: 1.0.0
 *
 * Requirements:
 * - FastLED library
 * - SumaShield hardware v0.5 or later
 * - NeoPixel SK6812MINI-E on GPIO 6 (future support)
 * - OLED display 72x40 (optional)
 *
 * Controls:
 * - Keys 0-7: Switch visual modes
 * - Knob 1 (ADC 26): Speed / Scale
 * - Knob 2 (ADC 27): Color / Brightness
 * - Rotary: Fine parameter adjustment
 * - Button A: Flash white (VJ effect)
 *
 * Author: Leo Kuroshita (Hugelton instruments)
 */

#include <I2S.h> // Dummy to avoid errors with I2S DAC
#include <FastLED.h>
#include <Wire.h>
#include <U8g2lib.h>

// Version information
#define PIXELSYNTH_VERSION_MAJOR 1
#define PIXELSYNTH_VERSION_MINOR 0
#define PIXELSYNTH_VERSION_PATCH 0
#define PIXELSYNTH_VERSION "1.0.0"

// ==========================================
// --- Pin Configuration (SumaShield) ---
// ==========================================
const int NEOPIXEL_PIN = 2;         // GPIO 2 (FastLED output)
const int PIN_KNOB_1 = 26;         // ADC 26 (Speed / Scale)
const int PIN_KNOB_2 = 27;         // ADC 27 (Color / Brightness)
const int PIN_ROTARY_SW = 9;       // Rotary encoder button
const int PIN_ROTARY_CLK = 10;     // Rotary encoder clock
const int PIN_ROTARY_DT = 11;      // Rotary encoder data
const int PIN_BTN_A = 7;           // Button A (flash effect)

// Key matrix (4x2 = 8 keys)
const int COL_PINS[4] = { 12, 13, 14, 15 };
const int ROW_PINS[2] = { 16, 17 };
const int KEY_MAP[8] = { 0, 4, 1, 5, 2, 6, 3, 7 };  // Physical layout

// ==========================================
// --- NeoPixel Configuration ---
// ==========================================
#define NUM_LEDS 256
#define MATRIX_WIDTH 16
#define MATRIX_HEIGHT 16
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 100

// Set this to false to test without LED matrix (for debugging)
#define ENABLE_FASTLED true

CRGB leds[NUM_LEDS];

// ==========================================
// --- Display Configuration ---
// ==========================================
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// ==========================================
// --- Global Variables ---
// ==========================================
uint16_t timePos = 0;
int currentMode = 0;
const int MAX_MODES = 8;

// Mode names for OLED display
const char* MODE_NAMES[8] = {
  "Noise Mesh",
  "Voronoi",
  "Plasma",
  "Ripples",
  "Matrix Rain",
  "Fire",
  "Tetris X",
  "Tetris Rot"
};

// Knob values
uint16_t knob1Value = 0;  // Speed / Scale
uint16_t knob2Value = 0;  // Color / Brightness
int8_t rotaryOffset = 0;   // Fine adjustment from rotary encoder

// Key states
bool key_state[8] = { false };
bool last_key_state[8] = { false };

// Rotary encoder state
volatile int rotary_counter = 0;

// OLED auto-sleep
volatile unsigned long last_activity_time = 0;
const unsigned long OLED_IDLE_TIMEOUT = 180000; // 3 minutes
bool oled_is_on = true;

// Flash effect state
bool flash_active = false;

// Tetris state
int8_t tetris_x = 8;        // X position (0-15)
int8_t tetris_rot = 0;     // Rotation (0-3)
bool tetris_btn_a_prev = false;

// ==========================================
// --- Helper Functions ---
// ==========================================

// 16x16 matrix XY coordinate conversion (zigzag wiring)
uint16_t XY(uint8_t x, uint8_t y) {
  if (y & 0x01) {
    return (y * MATRIX_WIDTH) + (MATRIX_WIDTH - 1 - x);
  } else {
    return (y * MATRIX_WIDTH) + x;
  }
}

// Key matrix scanning (SumaShield: COL=INPUT, ROW=OUTPUT)
void scanKeyMatrix() {
  for (int row = 0; row < 2; row++) {
    // Set current row LOW, others HIGH
    for (int r = 0; r < 2; r++) {
      digitalWrite(ROW_PINS[r], (r == row) ? LOW : HIGH);
    }
    delayMicroseconds(10);

    // Read all 4 columns
    for (int col = 0; col < 4; col++) {
      int key_idx = KEY_MAP[col + row * 4];
      bool pressed = (digitalRead(COL_PINS[col]) == LOW);
      key_state[key_idx] = pressed;
    }
  }
  last_activity_time = millis();
}

// Rotary encoder ISR
void rotary_isr() {
  static unsigned long last_irq = 0;
  static uint8_t last_clk = HIGH;

  if (millis() - last_irq < 10) {
    return;
  }

  uint8_t current_clk = digitalRead(PIN_ROTARY_CLK);

  if (last_clk == HIGH && current_clk == LOW) {
    uint8_t dt_state = digitalRead(PIN_ROTARY_DT);

    if (dt_state == LOW) {
      rotary_counter--;
    } else {
      rotary_counter++;
    }

    rotaryOffset = constrain(rotary_counter, -50, 50);
    last_activity_time = millis();
  }

  last_clk = current_clk;
  last_irq = millis();
}

// OLED drawing
void drawOLED() {
  unsigned long current_time = millis();
  unsigned long idle_time = current_time - last_activity_time;

  // Auto-sleep after 3 minutes
  if (idle_time > OLED_IDLE_TIMEOUT) {
    if (oled_is_on) {
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      oled_is_on = false;
    }
    return;
  }

  if (!oled_is_on) {
    oled_is_on = true;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr);

  // Show mode name
  u8g2.setCursor(0, 8);
  u8g2.print("M:");
  u8g2.print(currentMode);
  u8g2.print(" ");
  u8g2.print(MODE_NAMES[currentMode]);

  // Show parameter values
  u8g2.setCursor(0, 16);
  u8g2.print("K1:");
  u8g2.print(map(knob1Value, 0, 4095, 0, 100));

  u8g2.setCursor(36, 16);
  u8g2.print("K2:");
  u8g2.print(map(knob2Value, 0, 4095, 0, 255));

  // Show rotary offset
  u8g2.setCursor(0, 24);
  u8g2.print("ROT:");
  if (rotaryOffset >= 0) {
    u8g2.print("+");
    u8g2.print(rotaryOffset);
  } else {
    u8g2.print(rotaryOffset);
  }

  // Show flash effect indicator
  if (flash_active) {
    u8g2.setCursor(60, 8);
    u8g2.print("FLASH");
  }

  u8g2.sendBuffer();
}

// ==========================================
// --- Visual Modes ---
// ==========================================

// Mode 0: Noise Mesh
void drawNoiseMesh(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      uint8_t noise = inoise8((x * scale) + fineOffset, (y * scale) + fineOffset, timePos);
      uint8_t hue = noise + colorOffset;
      uint8_t bri = sin8(noise);
      leds[XY(x, y)] = CHSV(hue, 255, bri);
    }
  }
}

// Mode 1: Voronoi
void drawVoronoi(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  uint8_t p1x = beatsin8((timePos / 20) + fineOffset, 0, MATRIX_WIDTH - 1);
  uint8_t p1y = beatsin8((timePos / 25) + fineOffset, 0, MATRIX_HEIGHT - 1);
  uint8_t p2x = beatsin8((timePos / 15) + fineOffset + 64, 0, MATRIX_WIDTH - 1);
  uint8_t p2y = beatsin8((timePos / 18) + fineOffset + 64, 0, MATRIX_HEIGHT - 1);

  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      uint8_t dist1 = abs(x - p1x) + abs(y - p1y);
      uint8_t dist2 = abs(x - p2x) + abs(y - p2y);
      uint8_t minDist = min(dist1, dist2);
      minDist = (minDist * scale) / 10;

      uint8_t bri = 255 - (minDist * 10);
      uint8_t hue = colorOffset + (minDist * 5);
      leds[XY(x, y)] = CHSV(hue, 255, bri);
    }
  }
}

// Mode 2: Plasma
void drawPlasma(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      uint8_t value = sin8((x * scale) + timePos + fineOffset)
                    + sin8((y * scale) + timePos + fineOffset)
                    + sin8(((x + y) * scale) / 2 + timePos + fineOffset);
      leds[XY(x, y)] = CHSV(value / 3 + colorOffset, 255, 255);
    }
  }
}

// Mode 3: Ripples
void drawRipples(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      int dx = (x * 2) - 15;
      int dy = (y * 2) - 15;
      uint8_t dist = sqrt16(dx * dx + dy * dy) * scale / 5;

      uint8_t value = sin8(dist - timePos + (fineOffset * 5));
      leds[XY(x, y)] = CHSV(colorOffset + dist/2, 255, value);
    }
  }
}

// Mode 4: Matrix Rain
void drawMatrixRain(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      uint8_t noise = inoise8(x * 50, (y * scale) - timePos + fineOffset);
      uint8_t bri = (noise > 150) ? map(noise, 150, 255, 0, 255) : 0;
      leds[XY(x, y)] = CHSV(colorOffset, 255, bri);
    }
  }
}

// Mode 5: Fire
void drawFire(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      uint8_t noise = inoise8(x * scale, (y * scale) - timePos * 2, timePos / 2 + fineOffset);

      uint8_t fade = y * 15;
      uint8_t bri = qsub8(noise, fade);

      uint8_t hue = colorOffset + (noise / 4) - (y * 2);

      leds[XY(x, y)] = CHSV(hue, 255, bri);
    }
  }
}

// Mode 6: Tetris X (knob 1 controls X position)
void drawTetrisX(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  // Clear all LEDs first
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Draw 4x4 tetromino at tetris_x position
  int8_t base_x = constrain(tetris_x + fineOffset, 0, 12);
  int8_t base_y = 6;  // Center vertically

  // Simple 2x2 block (O-piece)
  for (int dy = 0; dy < 2; dy++) {
    for (int dx = 0; dx < 2; dx++) {
      int x = base_x + dx;
      int y = base_y + dy;
      if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
        leds[XY(x, y)] = CHSV(colorOffset, 255, 255);
      }
    }
  }
}

// Mode 7: Tetris Rotation (button A cycles rotation)
void drawTetrisRot(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  // Clear all LEDs first
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Draw tetromino based on rotation
  int8_t base_x = 8 + fineOffset;
  int8_t base_y = 6;

  // Rotation patterns for I-piece (4x1 vertical)
  // rot 0: vertical, rot 1: horizontal
  if (tetris_rot % 2 == 0) {
    // Vertical
    for (int dy = 0; dy < 4; dy++) {
      int y = base_y + dy - 1;
      if (y >= 0 && y < MATRIX_HEIGHT) {
        leds[XY(base_x, y)] = CHSV(colorOffset + (dy * 20), 255, 255);
      }
    }
  } else {
    // Horizontal
    for (int dx = 0; dx < 4; dx++) {
      int x = base_x + dx - 1;
      if (x >= 0 && x < MATRIX_WIDTH) {
        leds[XY(x, base_y)] = CHSV(colorOffset + (dx * 20), 255, 255);
      }
    }
  }
}

// ==========================================
// --- Main Setup ---
// ==========================================

void setup() {
  Serial.begin(115200);
  Serial.print("PixelSynth v");
  Serial.println(PIXELSYNTH_VERSION);
  Serial.println("16x16 LED Matrix Visual Synth for SumaShield");
  Serial.println();

  // Initialize NeoPixel on GPIO 2
#if ENABLE_FASTLED
  FastLED.addLeds<LED_TYPE, NEOPIXEL_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  Serial.println("FastLED initialized on GPIO 2");
#else
  Serial.println("FastLED disabled - testing key matrix only");
#endif

  // Initialize key matrix (SumaShield: COL=INPUT, ROW=OUTPUT)
  for (int i = 0; i < 4; i++) {
    pinMode(COL_PINS[i], INPUT);
  }
  for (int i = 0; i < 2; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], HIGH);
  }

  // Initialize rotary encoder
  pinMode(PIN_ROTARY_SW, INPUT_PULLUP);
  pinMode(PIN_ROTARY_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_DT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_CLK), rotary_isr, CHANGE);

  // Initialize button A
  pinMode(PIN_BTN_A, INPUT_PULLUP);

  // Initialize OLED
  Wire.setClock(400000);
  u8g2.begin();

  // Set initial activity time
  last_activity_time = millis();

  Serial.println("PixelSynth initialized");
  Serial.println("NeoPixel on GPIO 2");
  Serial.println("Key matrix test - press keys to test:");
  Serial.println();
}

// ==========================================
// --- Main Loop ---
// ==========================================

void loop() {
  // 1. Read knobs
  knob1Value = analogRead(PIN_KNOB_1);  // Speed / Scale / Tetris X
  knob2Value = analogRead(PIN_KNOB_2);  // Color / Brightness

  // Update tetris X position based on knob 1 (mode 6)
  if (currentMode == 6) {
    tetris_x = map(knob1Value, 0, 4095, 0, 15);
  }

  // 2. Scan key matrix for mode switching
  scanKeyMatrix();

  // Check for mode switch (key press)
  // Keys 0-7 switch to modes 0-7
  for (int i = 0; i < 8; i++) {
    if (key_state[i] && !last_key_state[i]) {
      currentMode = i;
      last_activity_time = millis();
      Serial.print("Key ");
      Serial.print(i);
      Serial.print(" pressed -> Mode: ");
      Serial.print(currentMode);
      Serial.print(" - ");
      Serial.println(MODE_NAMES[currentMode]);
    }
    last_key_state[i] = key_state[i];
  }

  // 3. Read button A (flash effect for modes 0-5, rotation for mode 7)
  bool btn_a_pressed = (digitalRead(PIN_BTN_A) == LOW);

  if (currentMode == 7) {
    // Mode 7: Button A cycles rotation
    if (btn_a_pressed && !tetris_btn_a_prev) {
      tetris_rot = (tetris_rot + 1) % 4;
      last_activity_time = millis();
      Serial.print("Tetris rotation: ");
      Serial.println(tetris_rot);
    }
    tetris_btn_a_prev = btn_a_pressed;
    flash_active = false;  // No flash in mode 7
  } else {
    // Modes 0-6: Button A triggers flash effect
    flash_active = btn_a_pressed;
  }

  if (flash_active || btn_a_pressed) {
    last_activity_time = millis();
  }

  // 4. Update time
  uint8_t speedVal = map(knob1Value, 0, 4095, 1, 30);
  timePos += speedVal;

  // 5. Map knobs to parameters
  uint8_t scaleVal = map(knob1Value, 0, 4095, 10, 100);
  uint8_t colorOffset = map(knob2Value, 0, 4095, 0, 255);

  // 6. Draw current mode
  if (!flash_active) {
    switch (currentMode) {
      case 0: drawNoiseMesh(scaleVal, colorOffset, rotaryOffset); break;
      case 1: drawVoronoi(scaleVal, colorOffset, rotaryOffset); break;
      case 2: drawPlasma(scaleVal, colorOffset, rotaryOffset); break;
      case 3: drawRipples(scaleVal, colorOffset, rotaryOffset); break;
      case 4: drawMatrixRain(scaleVal, colorOffset, rotaryOffset); break;
      case 5: drawFire(scaleVal, colorOffset, rotaryOffset); break;
      case 6: drawTetrisX(scaleVal, colorOffset, rotaryOffset); break;
      case 7: drawTetrisRot(scaleVal, colorOffset, rotaryOffset); break;
    }
  } else {
    // Flash white effect
    fill_solid(leds, NUM_LEDS, CRGB::White);
  }

  // 7. Update LED matrix
#if ENABLE_FASTLED
  FastLED.show();
#endif

  // 8. Update OLED display
  static unsigned long last_draw_time = 0;
  if (millis() - last_draw_time > 100) {  // 10fps (reduce CPU load)
    drawOLED();
    last_draw_time = millis();
  }

  // 9. Small delay
  delay(5);
}
