/*
 * SumaPiko
 *
 * Lo-fi music mangler based on pikocore
 * Adapted for SumaShield hardware (I2S PT8211, UART MIDI, key matrix, OLED)
 *
 * Original: https://github.com/schollz/pikocore
 * Author: Leo Kuroshita (Hugelton instruments)
 */

#include <I2S.h>

// Version information
#define SUMAPIKO_VERSION_MAJOR 1
#define SUMAPIKO_VERSION_MINOR 0
#define SUMAPIKO_VERSION_PATCH 0
#define SUMAPIKO_VERSION "1.0.0"
#include <Wire.h>
#include <U8g2lib.h>
#include <MIDI.h>

// UART MIDI (DIN MIDI only, no USB)
#define MIDI_SERIAL_PORT Serial1
MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL_PORT, MIDI_SERIAL);

// Helper headers from pikocore
#include "doth/audio2h.h"
#include "doth/button.h"
#include "doth/knob.h"
#include "doth/sequencer.h"
// midi_out.h removed (UART only, no USB MIDI out)
#include "doth/runningavg.h"
#include "doth/delay.h"
#include "doth/filter.h"

// Arduino IDE: Define Pico SDK constants
#define FLASH_PAGE_SIZE 256
#define CLOCK_RATE (SAMPLE_RATE * 8)

// ==========================================
// --- Pin Configuration (SumaShield) ---
// ==========================================
const int PIN_DATA = 19;     // I2S DATA
const int PIN_LOW  = 20;     // I2S BCLK

const int PIN_DECAY_ADC = 26; // Knob 0 (Function selector/Function B)
const int PIN_PITCH_ADC = 27; // Knob 1 (Function A)

const int PIN_BTN_A = 7;     // Button A (future: mute/play)
const int PIN_BTN_B = 8;     // Button B (future: rec/delete)

// Key matrix (4x2 = 8 keys)
const int COL_PINS[4] = { 12, 13, 14, 15 };
const int ROW_PINS[2] = { 16, 17 };
const int KEY_MAP[8] = { 0, 4, 1, 5, 2, 6, 3, 7 };  // Physical layout

// Rotary encoder
const int PIN_ROTARY_SW  = 9;
const int PIN_ROTARY_CLK = 10;
const int PIN_ROTARY_DT  = 11;

// OLED (72x40)
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// ==========================================
// --- Constants ---
// ==========================================
#define SAMPLE_RATE 31000  // Keep original pikocore sample rate
#define NUM_BUTTONS 8
#define NUM_KNOBS 2        // Reduced from 3
#define DISTORTION_MAX 30
#define VOLUME_REDUCE_MAX 30
#define HEAD_SHIFT 10      // crossfade time in samples (2^HEAD_SHIFT)
#define MAIN_LOOP_HZ 4
#define MAIN_LOOP_DELAY 50

// ==========================================
// --- Global Variables ---
// ==========================================

// Inputs
Button input_button[NUM_BUTTONS];
Knob input_knob[NUM_KNOBS];

// Audio tracking
uint8_t audio_now = 0;
uint8_t audio_clk = 0;
uint8_t audio_clk_thresh = 48;
bool do_mute = false;
uint8_t do_mute_debounce = 0;

// Note: MidiOut removed (UART only, no USB MIDI out)

// Sample tracking
uint16_t sample = 0;
uint16_t sample_beats = 8;
uint16_t sample_change = 0;
uint16_t sample_add = 0;
uint16_t sample_set = 0;
uint32_t phase_sample[] = {0, 0};
uint32_t phase_retrig = 0;
bool phase_head = 0;
uint32_t phase_xfade = 0;

// Beat tracking
uint16_t select_beat = 0;
uint16_t select_beat_freeze = 0;
bool direction[] = {1, 1};  // 0 = reverse, 1 = forward
bool base_direction = 1;    // 0 = reverse, 1 == forward
uint8_t volume_mod = 0;

// Volume/distortion/filter
uint8_t distortion = 0;
uint8_t volume_reduce = 0;
uint8_t filter_fc = LPF_MAX + 10;
uint8_t hpf_fc = 0;
uint8_t filter_q = 0;
uint8_t stretch_change = 0;
bool do_lock_clock = false;

// Beat tracking (beat = eighth-note)
uint32_t beat_counter = 0;
uint16_t bpm_set = BPM_SAMPLED;
uint32_t beat_thresh = 0;
uint32_t beat_num_total = 0;
bool beat_onset = false;
bool beat_led = 0;
bool btn_reset = 0;
bool soft_sync = 0;
bool is_syncing = false;
bool do_sync_play = false;

// Probabilities
uint8_t probability_jump = 0;
uint8_t probability_direction = 0;
uint8_t probability_retrig = 0;
uint8_t probability_gate = 0;
uint8_t probability_tunnel = 0;

// Retriggering / FX
bool fx_retrig = false;
bool btn_retrig = 0;
uint8_t retrig_sel = 4;
uint8_t retrig_count = 0;
uint8_t retrig_max = 2;
uint8_t retrig_filter = 0;
uint8_t retrig_filter_change = 0;
int8_t retrig_pitch_change = 0;
uint8_t retrig_volume_reduce = 0;
uint8_t button_on = NUM_BUTTONS;
uint8_t button_on2 = 3;
uint8_t button_filter = 0;
bool button_filter_on = false;
uint8_t retrig_volume_reduce_change = 0;
bool retrig_pitch_up = false;
bool retrig_pitch_down = false;
uint8_t syncing_clicks = 0;

// BPM configuring
bool flag_half_time = 0;  // specifies quarter note or not

// Noise gate
uint16_t noise_gate_val;
uint16_t noise_gate_thresh;
uint16_t noise_gate_thresh_use;
uint8_t noise_gate_fade = 0;

// Buttons
bool button_trigger[8] = {false, false, false, false,
                          false, false, false, false};

// Sequencer
Sequencer sequencer;

// MIDI note set for mapping
uint8_t midi_notes_set[8] = {36, 38, 40, 41, 43, 45, 47, 48};

int8_t midi_button1 = -1;
int8_t midi_button2 = -1;

// Rotary encoder
volatile int rotary_counter = 0;

// OLED auto-sleep
volatile unsigned long last_activity_time = 0;
const unsigned long OLED_IDLE_TIMEOUT = 180000; // 3 minutes
bool oled_is_on = true;

// I2S object
I2S i2s(OUTPUT);

// ==========================================
// --- Helper Functions ---
// ==========================================

// Parameter setting (from pikocore)
void param_set_bpm(uint16_t bpm, uint16_t &bpm_set_, uint32_t &beat_thresh_,
                   uint8_t &audio_clk_thresh_) {
  if (bpm > 360) {
    return;
  }
  bpm_set_ = bpm;
  double bpm_fudge = ((double)bpm) * 1.054234344 - 1.420118655;
  beat_thresh_ = round(((SAMPLE_RATE * 960.0)) / bpm_fudge);
  audio_clk_thresh = round(CLOCK_RATE * BPM_SAMPLED / 250.0 /
                           (SAMPLE_RATE / 1000.0) / bpm_fudge);
}

void param_set_volume(uint16_t knobval, uint8_t &distortion_,
                      uint8_t &volume_reduce_) {
  if (knobval < 2000) {
    distortion_ = 0;
    volume_reduce_ = (2000 - knobval) * (VOLUME_REDUCE_MAX + 3) / 2000;
  } else if (knobval > 3000) {
    volume_reduce_ = 0;
    distortion = (knobval - 3000) * DISTORTION_MAX / (4095 - 3000);
  } else {
    volume_reduce_ = 0;
    distortion = 0;
  }
}

int randint(int min, int max) {
  int MaxValue = max - min;
  int random_value =
      (int)((1.0 + MaxValue) * rand() /
            (RAND_MAX + 1.0));
  return (random_value + min);
}

// Key matrix scanning
void scanKeyMatrix() {
  for (int col = 0; col < 4; col++) {
    // Set current column LOW, others HIGH
    for (int c = 0; c < 4; c++) {
      digitalWrite(COL_PINS[c], (c == col) ? LOW : HIGH);
    }
    delayMicroseconds(10);

    // Read both rows
    for (int row = 0; row < 2; row++) {
      int key_idx = KEY_MAP[col + row * 4];
      bool pressed = (digitalRead(ROW_PINS[row]) == LOW);
      input_button[key_idx].Set(pressed);
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

    // Update BPM based on rotary
    uint16_t bpm_new = bpm_set + rotary_counter;
    if (bpm_new < 40) bpm_new = 40;
    if (bpm_new > 360) bpm_new = 360;
    param_set_bpm(bpm_new, bpm_set, beat_thresh, audio_clk_thresh);

    last_activity_time = millis();
  }

  last_clk = current_clk;
  last_irq = millis();
}

// MIDI callbacks
void midi_note_on(uint8_t note, uint8_t velocity) {
  // Map MIDI note to button (36-48 range)
  for (int i = 0; i < 8; i++) {
    if (note == 36 + i) {
      input_button[i].Set(true);
      if (midi_button1 > -1) {
        midi_button2 = i;
      } else {
        midi_button1 = i;
      }
      break;
    }
  }
  last_activity_time = millis();
}

void midi_note_off(uint8_t note) {
  for (int i = 0; i < 8; i++) {
    if (note == 36 + i) {
      input_button[i].Set(false);
      if (midi_button2 > -1) {
        midi_button2 = -1;
      } else {
        midi_button1 = -1;
      }
      break;
    }
  }
}

void midi_start() {
  do_mute = false;
  soft_sync = false;
  btn_reset = true;
  is_syncing = false;
  syncing_clicks = 0;
  last_activity_time = millis();
}

void midi_stop() {
  do_mute = true;
  soft_sync = false;
  btn_reset = false;
  last_activity_time = millis();
}

void midi_timing() {
  syncing_clicks++;
  if (syncing_clicks < 10) {
    is_syncing = true;
  }
  soft_sync = true;
  do_sync_play = true;
  last_activity_time = millis();
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

  // Show sample and beat
  u8g2.setCursor(0, 10);
  u8g2.print("S:");
  u8g2.print(sample);

  u8g2.setCursor(40, 10);
  u8g2.print("B:");
  u8g2.print(select_beat);

  // Show BPM
  u8g2.setCursor(0, 20);
  u8g2.print("BPM:");
  u8g2.print(bpm_set);

  // Show mute state
  if (do_mute) {
    u8g2.setCursor(0, 30);
    u8g2.print("MUTED");
  }

  u8g2.sendBuffer();
}

// ==========================================
// --- Core 0: UI & Control ---
// ==========================================

void setup() {
  Serial.begin(115200);
  Serial.print("SumaPiko v");
  Serial.println(SUMAPIKO_VERSION);
  Serial.println("Lo-fi music mangler based on pikocore");
  Serial.println("Adapted for SumaShield hardware");
  Serial.println();

  // Initialize UART MIDI (Arduino IDE uses uint16_t for baud rate)
  MIDI_SERIAL.begin((unsigned long)31250);

  // Initialize knobs (Arduino IDE: ADC auto-initialized)
  for (uint8_t i = 0; i < NUM_KNOBS; i++) {
    input_knob[i].Init(i, 50);
  }

  // Initialize buttons (key matrix will be scanned in loop)
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    input_button[i].Init(i, 10);
  }

  // Initialize key matrix
  for (int i = 0; i < 4; i++) {
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(COL_PINS[i], HIGH);
  }
  for (int i = 0; i < 2; i++) {
    pinMode(ROW_PINS[i], INPUT_PULLUP);
  }

  // Initialize rotary encoder
  pinMode(PIN_ROTARY_SW, INPUT_PULLUP);
  pinMode(PIN_ROTARY_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_DT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_CLK), rotary_isr, CHANGE);

  // Initialize buttons
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);

  // Note: MidiOut removed (UART only)

  // Initialize sequencer
  sequencer.Init();

  // Note: Flash save functionality removed for Arduino IDE
  // (requires Pico SDK flash operations)

  // Initialize OLED
  Wire.setClock(400000);
  u8g2.begin();

  // Initialize BPM
  param_set_bpm(BPM_SAMPLED, bpm_set, beat_thresh, audio_clk_thresh);

  // Noise gate
  noise_gate_thresh = SAMPLES_PER_BEAT * 4;
  noise_gate_thresh_use = noise_gate_thresh;

  // Set initial activity time
  last_activity_time = millis();

  // Note: No flash save functionality in Arduino IDE version
  // (requires Pico SDK flash operations)

  Serial.println("SumaPiko initialized");
}

void loop() {
  // Process UART MIDI
  if (MIDI_SERIAL.read()) {
    midi::MidiType type = MIDI_SERIAL.getType();
    byte note = MIDI_SERIAL.getData1();
    byte velocity = MIDI_SERIAL.getData2();

    switch (type) {
      case midi::NoteOn:
        if (velocity > 0) midi_note_on(note, velocity);
        else midi_note_off(note);
        break;
      case midi::NoteOff:
        midi_note_off(note);
        break;
      case midi::Start:
        midi_start();
        break;
      case midi::Continue:
        midi_start();
        break;
      case midi::Stop:
        midi_stop();
        break;
      case midi::Clock:
        midi_timing();
        break;
    }
  }

  // Scan key matrix
  scanKeyMatrix();

  // Read knobs
  for (uint8_t i = 0; i < NUM_KNOBS; i++) {
    input_knob[i].Read();
  }

  // Update OLED
  static unsigned long last_draw_time = 0;
  if (millis() - last_draw_time > 33) {  // ~30fps
    drawOLED();
    last_draw_time = millis();
  }

  // Small delay
  delay(MAIN_LOOP_DELAY);
}

// ==========================================
// --- Core 1: Audio Output (I2S) ---
// ==========================================

void setup1() {
  // Initialize I2S
  i2s.setBCLK(PIN_LOW);
  i2s.setDATA(PIN_DATA);
  i2s.begin(SAMPLE_RATE);

  // Initialize filter state
  x1_f = 0;
  x2_f = 0;
  y1_f = 0;
  y2_f = 0;

  Serial.println("I2S audio initialized");
}

void loop1() {
  // Beat timing
  beat_counter++;
  if ((!is_syncing && beat_counter >= beat_thresh) || btn_reset || soft_sync) {
    soft_sync = false;
    beat_num_total++;
    beat_counter = 0;
    beat_onset = true;
    beat_led = 1 - beat_led;
    noise_gate_val = 0;

    if (do_mute_debounce > 0) {
      do_mute_debounce--;
    }

    // Check button states
    if (button_on < NUM_BUTTONS) {
      if (!input_button[button_on].On()) {
        button_on = NUM_BUTTONS;
        button_on2 = NUM_BUTTONS;
        select_beat_freeze = 0;
        button_filter_on = false;
        retrig_volume_reduce = 0;
        retrig_volume_reduce_change = 0;

        if (btn_reset) {
          retrig_count = retrig_max;
        }
      }
    } else if (do_mute_debounce == 0) {
      for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        if (input_button[i].On()) {
          if (button_on >= NUM_BUTTONS) {
            select_beat_freeze = (select_beat / NUM_BUTTONS) * NUM_BUTTONS;
          }
          button_on = i;
          break;
        }
      }
    }

    // Check button 2
    if (button_on2 < NUM_BUTTONS) {
      if (!input_button[button_on2].On()) {
        button_on2 = NUM_BUTTONS;
        button_filter_on = false;
      }
    }

    if (!btn_retrig) {
      if (button_on < NUM_BUTTONS && do_mute_debounce == 0) {
        for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
          if (i == button_on) continue;
          if (input_button[i].On()) {
            btn_retrig = true;
            button_on2 = i;
          }
        }
      } else {
        if (randint(0, 254) < probability_retrig) {
          btn_retrig = true;
        }
      }
    }

    if (!fx_retrig) {
      if (btn_retrig && !fx_retrig) {
        fx_retrig = true;
        uint8_t r1 = randint(0, 100);
        uint8_t r2 = randint(0, 100);
        uint8_t r3 = randint(0, 100);
        uint8_t r4 = randint(0, 100);
        retrig_count = 0;

        if (button_on2 >= NUM_BUTTONS) {
          retrig_sel = randint(2, 16);
        } else {
          switch (button_on2) {
            case 0: retrig_sel = randint(0, 2); break;
            case 1: retrig_sel = randint(2, 4); break;
            case 2: retrig_sel = randint(4, 6); break;
            case 3: retrig_sel = randint(6, 8); break;
            case 4: retrig_sel = randint(8, 10); break;
            case 5: retrig_sel = randint(10, 12); break;
            case 6: retrig_sel = randint(12, 14); break;
            case 7: retrig_sel = randint(14, 16); break;
          }
        }

        retrig_max = randint(3, 16);
        if (retrig_sel < 6) {
          retrig_max = retrig_max / 2;
        } else if (retrig_sel > 11) {
          retrig_max = retrig_max * 2;
        }

        if (r1 <= 15) {
          retrig_pitch_up = true;
        } else if (r2 <= 15) {
          retrig_pitch_down = true;
        }

        if (r3 < 30) {
          retrig_filter = retrig_max;
          retrig_filter_change = (LPF_MAX - 10) / retrig_max;
        }

        if (r4 < 20 && retrig_sel > 6) {
          retrig_volume_reduce = retrig_max;
          if (retrig_volume_reduce > 5) {
            retrig_volume_reduce = 5;
          }
          retrig_volume_reduce_change = 1;
          if (randint(1, 100) < 30) {
            retrig_volume_reduce_change = 2;
            retrig_volume_reduce = 1;
          }
        }

        audio_clk = audio_clk_thresh - 1;
        phase_retrig = (retrigs[retrig_sel] << flag_half_time) - 1;
      }
    }
  }

  // Disable beat interrupts during fx
  if (fx_retrig) {
    beat_onset = false;
  }

  // Clock when to change samples
  audio_clk++;
  if (audio_clk == (audio_clk_thresh + retrig_pitch_change + stretch_change) ||
      beat_onset) {
    audio_clk = 0;

    if (beat_onset && fx_retrig == false) {
      bool do_switch_heads = true;

      if (probability_tunnel > 0) {
        if (randint(0, 255) < probability_tunnel) {
          sample_add = randint(0, NUM_SAMPLES);
        } else {
          sample_add = 0;
        }
      } else {
        sample_add = 0;
      }

      if (sample_set != sample_change) {
        sample_set = sample_change;
      }

      sample = (sample_set + sample_add) % NUM_SAMPLES;
      sample_beats = raw_beats(sample);

      beat_onset = false;

      if (do_lock_clock) {
        select_beat = beat_num_total % sample_beats;
      } else {
        select_beat++;
      }

      if (flag_half_time) {
        select_beat++;
        if (select_beat % 2 > 0)
          select_beat++;
      }

      if (select_beat < 0) {
        do_switch_heads = false;
        select_beat = sample_beats - 1;
      }
      if (select_beat >= sample_beats) {
        do_switch_heads = false;
        select_beat = 0;
      }

      // Random jumps
      if (probability_jump > 0) {
        if (randint(0, 255) < probability_jump) {
          select_beat = randint(0, sample_beats - 1);
        }
      }

      // Random gate
      if (probability_gate > 0) {
        if (randint(0, 255) < probability_gate) {
          noise_gate_thresh_use = SAMPLES_PER_BEAT * randint(800, 1000) / 1000;
        } else {
          noise_gate_thresh_use = noise_gate_thresh;
        }
      } else {
        noise_gate_thresh_use = noise_gate_thresh;
      }

      if (btn_reset) {
        btn_reset = 0;
        select_beat = 0;
      }

      // Hold button down to play that beat
      if (button_on < NUM_BUTTONS) {
        select_beat = (button_on + select_beat_freeze) % sample_beats;
        sequencer.Record(select_beat);
      }

      // MidiOut removed (UART only)
      // To send MIDI out via UART, use: MIDI_SERIAL.noteOn(36 + (select_beat % 8), 127);

      if (do_switch_heads) {
        phase_head = 1 - phase_head;
        phase_xfade = 1 << HEAD_SHIFT;
      }
      phase_sample[phase_head] =
          select_beat * (SAMPLES_PER_BEAT << flag_half_time);

      // Random direction for the new head
      if (probability_direction > 0) {
        uint8_t r1 = randint(0, 255);
        if (direction[phase_head] == base_direction) {
          if (r1 < probability_direction) {
            direction[phase_head] = 1 - base_direction;
          }
        } else {
          if (r1 > probability_direction) {
            direction[phase_head] = base_direction;
          }
        }
      } else {
        direction[phase_head] = base_direction;
      }
    } else {
      // Update the sample
      noise_gate_val++;
      if (noise_gate_val < 10 & noise_gate_fade > 0) {
        noise_gate_fade--;
      } else if (noise_gate_val > noise_gate_thresh_use) {
        if (noise_gate_val % 100 == 0) {
          if (noise_gate_fade < 8) {
            noise_gate_fade++;
          }
        }
      }

      for (uint8_t i = 0; i < 2; i++) {
        if (direction[i]) {
          phase_sample[i]++;
          if (phase_sample[i] == raw_len(sample) - 1) {
            phase_sample[i] = 0;
          }
        } else {
          if (phase_sample[i] == 0) {
            phase_sample[i] = raw_len(sample) - 2;
          } else {
            phase_sample[i]--;
          }
        }
      }
    }

    if (fx_retrig) {
      phase_retrig++;

      noise_gate_fade = 0;
      noise_gate_val = 0;

      if (phase_retrig % (retrigs[retrig_sel] << flag_half_time) == 0) {
        retrig_count++;
        if (retrig_filter > 0) {
          retrig_filter--;
        }

        // MidiOut removed (UART only)
        // To send MIDI out via UART, use: MIDI_SERIAL.noteOn(36 + (select_beat % 8), velocity);

        if (retrig_volume_reduce_change == 1 && retrig_volume_reduce > 0) {
          if (retrig_sel > 11) {
            if (retrig_count % 2 == 0) {
              retrig_volume_reduce--;
            }
          } else {
            retrig_volume_reduce--;
          }
        } else if (retrig_volume_reduce_change == 2 &&
                   retrig_volume_reduce < 8 && retrig_count % 2 == 0) {
          if (retrig_sel > 11) {
            if (retrig_count % 4 == 0) {
              retrig_volume_reduce--;
            }
          } else {
            retrig_volume_reduce--;
          }
        }

        if (retrig_pitch_up) {
          retrig_pitch_change++;
        } else if (retrig_pitch_down) {
          retrig_pitch_change--;
        }

        if (retrig_count >= retrig_max) {
          retrig_filter = 0;
          retrig_pitch_up = false;
          retrig_pitch_down = false;
          retrig_pitch_change = 0;
          retrig_volume_reduce = 0;
          retrig_volume_reduce_change = 0;
          button_filter_on = false;
          fx_retrig = false;
          btn_retrig = false;
        }

        phase_head = 1 - phase_head;
        phase_xfade = 1 << HEAD_SHIFT;
        phase_sample[phase_head] =
            select_beat * (SAMPLES_PER_BEAT << flag_half_time);
        phase_retrig = 0;
      }
    }

    // Determine sample
    if (phase_xfade == 0) {
      audio_now = raw_val(sample, phase_sample[phase_head]);
    } else {
      phase_xfade--;

      uint32_t u = (uint32_t)raw_val(sample, phase_sample[phase_head]);
      u = u * ((1 << HEAD_SHIFT) - phase_xfade);

      uint32_t v = (uint32_t)raw_val(sample, phase_sample[1 - phase_head]);
      v = v * phase_xfade;

      u = (u + v) >> HEAD_SHIFT;

      audio_now = (uint8_t)u;
    }

    // Volume processing
    if (volume_reduce >= VOLUME_REDUCE_MAX) audio_now = 128;
    if (audio_now != 128) {
      // Distortion / wave-folding
      if (distortion > 0) {
        if (audio_now > 128) {
          if (audio_now < (255 - distortion)) {
            audio_now += distortion;
          } else {
            audio_now = 255 - distortion;
          }
          audio_now = 128 + ((audio_now - 128) / ((distortion >> 4) + 1));
        } else {
          if (audio_now > distortion) {
            audio_now -= distortion;
          } else {
            audio_now = distortion - audio_now;
          }
          audio_now = 128 - ((128 - audio_now) / ((distortion >> 4) + 1));
        }
      }

      // Reduce volume
      if (volume_reduce > 0) {
        if (audio_now > 128) {
          audio_now = audio_now - (volume_reduce);
          if (audio_now < 128) audio_now = 128;
        } else {
          audio_now = audio_now + (volume_reduce);
          if (audio_now > 128) audio_now = 128;
        }
      }

      if ((volume_mod + retrig_volume_reduce + noise_gate_fade) > 0 &&
          audio_now != 128) {
        if (audio_now > 128) {
          audio_now = ((audio_now - 128) >>
                       (volume_mod + retrig_volume_reduce + noise_gate_fade)) +
                      128;
        } else {
          audio_now =
              128 - ((128 - audio_now) >>
                     (volume_mod + retrig_volume_reduce + noise_gate_fade));
        }
      }
    }

    // Filter
    if ((filter_fc - (retrig_filter * retrig_filter_change) - button_filter) <=
        LPF_MAX) {
      audio_now = (uint8_t)filter_lpf(
          (int64_t)audio_now,
          (filter_fc - (retrig_filter * retrig_filter_change) - button_filter),
          filter_q);
    }
  }

  // Check mute state
  if ((!do_sync_play && is_syncing) || do_mute) {
    // Output silence (center value for I2S)
    int16_t sample_16 = 0;
    i2s.write(sample_16);
    i2s.write(sample_16);  // Stereo (both channels)
    return;
  }

  // Convert 8-bit unsigned to 16-bit signed for I2S
  // 0-255 (128=center) → -32768 to 32767 (0=center)
  int16_t sample_16 = (int16_t)((audio_now - 128) * 256);

  // Write to I2S (stereo)
  i2s.write(sample_16);
  i2s.write(sample_16);
}
