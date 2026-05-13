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

// Tetris state for mode 6
int8_t tetris_x = 8;        // Current piece X position
int8_t tetris_y = 0;        // Current piece Y position
int8_t tetris_rot = 0;     // Current piece rotation (0-3)
int8_t tetris_type = 0;     // Current piece type (0-6)
uint16_t tetris_score = 0;  // Score
bool tetris_game_over = false;
uint32_t tetris_drop_timer = 0;
const uint16_t TETRIS_DROP_SPEED = 10; // Frames between auto-drop

// Tetromino definitions (4x4 grids, 1=filled, 0=empty)
// 7 types: I(0), O(1), T(2), S(3), Z(4), J(5), L(6)
const uint8_t TETROMINOES[7][4][4] = {
  // I (4x1 vertical)
  {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}},
  // O (2x2 square)
  {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
  // T (T-shape)
  {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // S (S-shape)
  {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
  // Z (Z-shape)
  {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // J (J-shape)
  {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // L (L-shape)
  {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}
};

const uint8_t TETROMINO_COLORS[7] = {
  0,   // I: Red
  40,  // O: Gold
  80,  // T: Magenta
  120, // S: Green
  160, // Z: Blue
  200, // J: Orange
  240  // L: Purple
};

// Game board (16x16, 0=empty, 1-7=locked pieces)
uint8_t board[16][16] = {0};

// PONG state for mode 7
float pong_ball_x = 8.0;
float pong_ball_y = 8.0;
float pong_ball_vx = 0.5;
float pong_ball_vy = 0.3;
float pong_paddle_x = 6.0;
uint16_t pong_score = 0;
bool pong_game_over = false;

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

// Rotary encoder button ISR (for hard drop in Tetris)
void rotary_button_isr() {
  static unsigned long last_irq = 0;

  if (millis() - last_irq < 50) {  // 50ms debounce
    return;
  }

  // Mode 6: Hard drop
  if (currentMode == 6 && !tetris_game_over) {
    // Drop until collision
    while (isValidPosition(tetris_x, tetris_y + 1, tetris_rot, tetris_type)) {
      tetris_y++;
    }
    // Lock piece
    lockPiece();
    clearLines();
    spawnPiece();
  }

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

// Mode 6: Tetris (playable game)
// This is a simple Tetris with rotary for X movement, click for hard drop
void drawTetrisGame(uint8_t scale, uint8_t colorOffset, int8_t fineOffset) {
  // Clear all LEDs first
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Draw game board (locked pieces)
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      if (board[y][x] != 0) {
        uint8_t hue = TETROMINO_COLORS[board[y][x] - 1];
        leds[XY(x, y)] = CHSV(hue, 255, 200);
      }
    }
  }

  // Draw current falling piece
  uint8_t hue = TETROMINO_COLORS[tetris_type];
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (TETROMINOES[tetris_type][py][px] == 1) {
        int8_t draw_x = tetris_x + px;
        int8_t draw_y = tetris_y + py;
        if (draw_x >= 0 && draw_x < 16 && draw_y >= 0 && draw_y < 16) {
          leds[XY(draw_x, draw_y)] = CHSV(hue, 255, 255);
        }
      }
    }
  }

  // Draw score on top row (if space available)
  if (tetris_score > 0) {
    uint16_t score_digits = tetris_score;
    int8_t digit_pos = 15;
    while (score_digits > 0 && digit_pos >= 0) {
      uint8_t digit = score_digits % 10;
      if (digit > 0) {
        leds[XY(digit_pos, 0)] = CHSV(0, 0, 150); // Blue for score
      }
      score_digits /= 10;
      digit_pos--;
    }
  }
}

// ==========================================
// --- Tetris Game Logic (Mode 6) ---
// ==========================================

// Initialize Tetris game
void initTetris() {
  // Clear board
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      board[y][x] = 0;
    }
  }
  tetris_score = 0;
  tetris_game_over = false;
  tetris_x = 7;  // Center
  tetris_y = 0;
  tetris_rot = 0;
  tetris_type = random(7);  // Random piece
  tetris_drop_timer = 0;
}

// Check if current piece position is valid
bool isValidPosition(int8_t x, int8_t y, int8_t rot, int8_t type) {
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (TETROMINOES[type][py][px] == 1) {
        int8_t check_x = x + px;
        int8_t check_y = y + py;
        // Check bounds
        if (check_x < 0 || check_x >= 16 || check_y < 0 || check_y >= 16) {
          return false;
        }
        // Check collision with board
        if (board[check_y][check_x] != 0) {
          return false;
        }
      }
    }
  }
  return true;
}

// Lock current piece to board
void lockPiece() {
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (TETROMINOES[tetris_type][py][px] == 1) {
        int8_t lock_x = tetris_x + px;
        int8_t lock_y = tetris_y + py;
        if (lock_x >= 0 && lock_x < 16 && lock_y >= 0 && lock_y < 16) {
          board[lock_y][lock_x] = tetris_type + 1;  // Store type + 1
        }
      }
    }
  }
}

// Check and clear complete lines
void clearLines() {
  uint8_t lines_cleared = 0;

  for (int y = 15; y >= 0; y--) {
    bool line_full = true;
    for (int x = 0; x < 16; x++) {
      if (board[y][x] == 0) {
        line_full = false;
        break;
      }
    }

    if (line_full) {
      // Clear this line and move everything down
      for (int yy = y; yy > 0; yy--) {
        for (int xx = 0; xx < 16; xx++) {
          board[yy][xx] = board[yy - 1][xx];
        }
      }
      // Clear top line
      for (int xx = 0; xx < 16; xx++) {
        board[0][xx] = 0;
      }
      lines_cleared++;
      y++;  // Check this line again since we shifted down
    }
  }

  if (lines_cleared > 0) {
    tetris_score += lines_cleared * 100;
    Serial.print("Lines cleared: ");
    Serial.print(lines_cleared);
    Serial.print(" Score: ");
    Serial.println(tetris_score);
  }
}

// Spawn new piece
void spawnPiece() {
  tetris_x = 7;
  tetris_y = 0;
  tetris_rot = 0;
  tetris_type = random(7);

  // Check if spawn position is valid (game over check)
  if (!isValidPosition(tetris_x, tetris_y, tetris_rot, tetris_type)) {
    tetris_game_over = true;
    Serial.println("GAME OVER!");
  }
}

// Update Tetris game (called every frame in mode 6)
void updateTetrisGame() {
  static bool first_run = true;
  if (first_run) {
    initTetris();
    first_run = false;
  }

  if (tetris_game_over) {
    // Flash game over pattern
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    delay(100);
    return;
  }

  // Auto-drop
  tetris_drop_timer++;
  if (tetris_drop_timer >= TETRIS_DROP_SPEED) {
    tetris_drop_timer = 0;

    // Try to move down
    if (isValidPosition(tetris_x, tetris_y + 1, tetris_rot, tetris_type)) {
      tetris_y++;
    } else {
      // Lock piece and spawn new one
      lockPiece();
      clearLines();
      spawnPiece();
    }
  }
}

// ==========================================
// --- PONG Game Logic (Mode 7) ---
// ==========================================
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
  attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_SW), rotary_button_isr, FALLING);

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
  knob1Value = analogRead(PIN_KNOB_1);  // Speed / Scale (not used in Tetris)
  knob2Value = analogRead(PIN_KNOB_2);  // Color / Brightness

  // Update Tetris controls (mode 6 only)
  if (currentMode == 6 && !tetris_game_over) {
    // Rotary controls X position
    static int last_rotary_counter = 0;
    if (rotary_counter != last_rotary_counter) {
      // Try to move piece
      int8_t new_x = tetris_x + (rotary_counter - last_rotary_counter);
      if (isValidPosition(new_x, tetris_y, tetris_rot, tetris_type)) {
        tetris_x = new_x;
      }
      last_rotary_counter = rotary_counter;
    }
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
      case 6: drawTetrisGame(scaleVal, colorOffset, rotaryOffset); break;
      case 7: drawTetrisRot(scaleVal, colorOffset, rotaryOffset); break;
    }
  } else {
    // Flash white effect
    fill_solid(leds, NUM_LEDS, CRGB::White);
  }

  // Update Tetris game state (only in mode 6)
  if (currentMode == 6 && !flash_active) {
    updateTetrisGame();
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
