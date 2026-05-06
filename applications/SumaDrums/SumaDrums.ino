#include <I2S.h>
#include <Wire.h>
#include <U8g2lib.h>

// USB MIDI対応（TinyUSBスタック使用）
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// MIDIインスタンス作成（USB + シリアル）
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);

// UART0でレガシーMIDI DIN（GPIO0=TX, GPIO1=RX）
#define MIDI_SERIAL_PORT Serial1
MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL_PORT, MIDI_SERIAL);

// --- ピン設定 ---
const int PIN_DATA = 19;
const int PIN_LOW = 20;

const int PIN_DECAY_ADC = 26;
const int PIN_PITCH_ADC = 27;
const int PIN_BTN_A = 7;  // Play/Stop
const int PIN_BTN_B = 8;  // Record Mode Toggle
// BTN B (Shift) は廃止

const int PIN_ROTARY_SW = 9;
const int PIN_ROTARY_CLK = 10;
const int PIN_ROTARY_DT = 11;

// const int PIN_LED        = 25;  // Pico オンボードLED (テンポ同期用)

// キーマトリクス (4 Col x 2 Row = 8 keys)
const int COL_PINS[4] = { 12, 13, 14, 15 };
const int ROW_PINS[2] = { 16, 17 };
const int KEY_MAP[8] = { 0, 4, 1, 5, 2, 6, 3, 7 };  // 物理配置補正

// OLED (72x40)
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// ==========================================
// --- プリセット・パターン (8種類 x 4Track x 16Step) ---
// ==========================================
const uint8_t PRESET_PATTERNS[8][4][16] = {
  // Pattern 0: Basic Groove
  {
    { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0 },  // Kick
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },  // Snare
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  // HH Closed
    { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 }   // HH Open
  },
  // Pattern 1: House (4-on-the-floor)
  { { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
  // Pattern 2: Breakbeat
  { { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 },
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
  // Pattern 3: Electro
  { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0 } },
  // Pattern 4: Trap Style
  { { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
  // Pattern 5: Dembow / Reggaeton
  { { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0 },
    { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
  // Pattern 6: Minimal
  { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
  // Pattern 7: Busy Fill
  { { 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 } }
};

// --- 再生中のシーケンス・データ ---
volatile uint8_t seq[8][16];  // 8トラック全て使用

// --- UI 状態 ---
bool key_state[8] = { 0 };
bool last_key_state[8] = { 0 };

bool is_playing = true;
bool is_recording = false;  // Recordモード

bool last_btn_a = HIGH;
bool last_btn_b = HIGH;
int current_step = 0;
unsigned long last_step_time = 0;
unsigned long last_draw_time = 0;

unsigned long led_on_time = 0;  // LED消灯タイマー用

volatile int current_bpm = 120;

// --- DSP パラメータ ---
volatile float envs[8] = { 0.0 };    // 8トラックエンベロープ
volatile float phases[8] = { 0.0 };  // 8トラック位相
volatile bool phase_reset_request[8] = { false };  // 位相リセット要求（Core 1で処理）
volatile float hh_last_noise = 0.0;
volatile float metronome_env = 0.0;  // メトロノームエンベロープ

// トラック個別パラメータ
volatile float track_pitch[8] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
volatile float track_decay[8] = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };

volatile int last_triggered_track = -1;  // 最後に叩いた楽器
bool is_manual_trigger = false;          // マニュアルトリガーかどうか

// ADC変化検出用
int last_adc_decay = 0;
int last_adc_pitch = 0;

// 編集中のトラック管理（Decay/Pitch独立）
int last_edited_decay_track = -1;        // Decayで最後に編集した楽器
int last_edited_pitch_track = -1;        // Pitchで最後に編集した楽器
unsigned long last_decay_edit_time = 0;  // Decay最終編集時刻
unsigned long last_pitch_edit_time = 0;  // Pitch最終編集時刻

// ADCノイズ対策
#define ADC_THRESHOLD 15  // ADC変化検出のしきい値（感度向上）
#define ADC_MAX_CHANGE 500  // 1フレームでの最大変化（緩和してノブ操作を反映）

// ボタン状態管理
enum BtnState { IDLE,
                PRESSING,
                LONG_PRESSING };
BtnState btn_a_state = IDLE;
BtnState btn_b_state = IDLE;
unsigned long btn_a_press_time = 0;
unsigned long btn_b_press_time = 0;

// Cymbal用変数
float cymbal_phases[6] = { 0 };  // TR-808風にするため6つに増加
float cymbal_last_sample = 0;

// メトロノーム用
float metronome_phase = 0;

// OLED焼きつき防止用：アイドル検出
volatile unsigned long last_activity_time = 0;
const unsigned long OLED_IDLE_TIMEOUT = 180000; // 3分（ミリ秒）
bool oled_is_on = true;

// 爆速化のための32bit定数
const float PI_F = 3.14159265f;
const float TWO_PI_F = 6.2831853f;
// 2π / 22050 をあらかじめ計算しておく（割り算を撲滅）
const float PHASE_INC_MULT = TWO_PI_F / 22050.0f; 

// トラック割り当て
// 0-7: 全てルーパー（シーケンサー制御）
//   0: Kick, 1: Snare, 2: Closed Hat, 3: Open Hat
//   4: Tom1, 5: Tom2, 6: Clap, 7: Cymbal

// MIDI用グローバル変数
volatile bool  g_midi_key_state[8] = { false };  // MIDIから来るキー状態
volatile int   g_midi_note_map[8] = {60, 62, 64, 65, 67, 69, 71, 72}; // MIDIノート番号マップ（C4-C5）
// C4(60)=Kick, D4(62)=Snare, E4(64)=ClosedHat, F4(65)=OpenHat
// G4(67)=Tom1, A4(69)=Tom2, B4(71)=Clap, C5(72)=Cymbal

I2S i2s(OUTPUT);
const int SAMPLE_RATE = 22050; 

// ==========================================
// パターン読み込み関数
// ==========================================
void loadPattern(int ptn_idx) {
  // 8トラック全てクリアしてからロード
  for (int t = 0; t < 8; t++) {
    for (int s = 0; s < 16; s++) {
      if (t < 4) {
        // 0-3トラックはプリセットからロード
        seq[t][s] = PRESET_PATTERNS[ptn_idx][t][s];
      } else {
        // 4-7トラックは空にする
        seq[t][s] = 0;
      }
    }
  }
}

// MIDIメッセージ処理関数（共通）
void processMIDIMessage(midi::MidiType type, byte note, byte velocity) {
  if (type == midi::NoteOn) {
    if (velocity > 0) {
      // Note On: ノート番号をドラムトラックにマッピング
      for (int i = 0; i < 8; i++) {
        if (note == g_midi_note_map[i]) {
          g_midi_key_state[i] = true;
          break;
        }
      }
    } else {
      // Velocity 0 = Note Off
      for (int i = 0; i < 8; i++) {
        if (note == g_midi_note_map[i]) {
          g_midi_key_state[i] = false;
          break;
        }
      }
    }
  } else if (type == midi::NoteOff) {
    // Note Off: ノート番号をドラムトラックにマッピング
    for (int i = 0; i < 8; i++) {
      if (note == g_midi_note_map[i]) {
        g_midi_key_state[i] = false;
        break;
      }
    }
  }
}

// ==========================================
// 割り込み: ロータリーエンコーダ
// ==========================================
void rotary_isr() {
  static unsigned long last_irq = 0;
  if (millis() - last_irq > 5) {
    if (digitalRead(PIN_ROTARY_DT) != digitalRead(PIN_ROTARY_CLK)) current_bpm++;
    else current_bpm--;
    if (current_bpm < 40) current_bpm = 40;
    if (current_bpm > 300) current_bpm = 300;

    // ロータリ操作があったのでアクティビティ時間を更新
    last_activity_time = millis();
  }
  last_irq = millis();
}

// ==========================================
// Core 0: UI, マトリクス, OLED
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
  pinMode(PIN_ROTARY_SW, INPUT_PULLUP);
  pinMode(PIN_ROTARY_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_DT, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);

  for (int i = 0; i < 4; i++) pinMode(COL_PINS[i], INPUT);
  for (int i = 0; i < 2; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], HIGH);
  }

  attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_CLK), rotary_isr, FALLING);
  analogReadResolution(12);

  Wire.setSDA(4);
  Wire.setSCL(5);
  u8g2.begin();

  // ADC初期値読み取り
  last_adc_decay = analogRead(PIN_DECAY_ADC);
  last_adc_pitch = analogRead(PIN_PITCH_ADC);

  // 起動時に全て空白のパターンをロード
  for (int t = 0; t < 8; t++) {
    for (int s = 0; s < 16; s++) {
      seq[t][s] = 0;  // 全て空白
    }
  }

  // 初期化時にアクティビティ時間をセット
  last_activity_time = millis();

  Serial.println("SumaDrums with USB MIDI + Serial MIDI - Ready!");
}

void loop() {
  // 0. MIDIメッセージ処理（USB + シリアル）
  // USB MIDI
  if (MIDI_USB.read()) {
    last_activity_time = millis(); // MIDI受信でアクティビティ更新

    midi::MidiType type = MIDI_USB.getType();
    byte note = MIDI_USB.getData1();
    byte velocity = MIDI_USB.getData2();

    processMIDIMessage(type, note, velocity);
  }

  // シリアルMIDI（レガシーDIN）
  if (MIDI_SERIAL.read()) {
    last_activity_time = millis(); // MIDI受信でアクティビティ更新

    midi::MidiType type = MIDI_SERIAL.getType();
    byte note = MIDI_SERIAL.getData1();
    byte velocity = MIDI_SERIAL.getData2();

    processMIDIMessage(type, note, velocity);
  }

  // 1. ADC - しきい値以上の変化時だけ「現在編集中の楽器」のパラメータを更新
  int current_adc_decay = analogRead(PIN_DECAY_ADC);
  int current_adc_pitch = analogRead(PIN_PITCH_ADC);

  // ADC変化量を計算
  int decay_diff = current_adc_decay - last_adc_decay;
  int pitch_diff = current_adc_pitch - last_adc_pitch;

  // Decay ADCが変わった時（しきい値以上、かつ大幅な変化でない）
  if (abs(decay_diff) > ADC_THRESHOLD && abs(decay_diff) < ADC_MAX_CHANGE) {
    last_activity_time = millis();  // ADC操作でアクティビティ更新
    // 楽器を叩いている時は、その楽器を「編集中」として記憶
    if (last_triggered_track >= 0 && last_triggered_track < 8) {
      // まだ編集されていない時だけ記憶
      if (last_edited_decay_track != last_triggered_track) {
        last_edited_decay_track = last_triggered_track;
      }
    }

    // 編集中の楽器のパラメータを更新
    if (last_edited_decay_track >= 0 && last_edited_decay_track < 8) {
      track_decay[last_edited_decay_track] = map(current_adc_decay, 0, 4095, 1000, 0) / 1000.0f;
    }
    last_adc_decay = current_adc_decay;
    last_decay_edit_time = millis();
  }

  // Pitch ADCが変わった時（しきい値以上、かつ大幅な変化でない）
  if (abs(pitch_diff) > ADC_THRESHOLD && abs(pitch_diff) < ADC_MAX_CHANGE) {
    last_activity_time = millis();  // ADC操作でアクティビティ更新
    // 楽器を叩いている時は、その楽器を「編集中」として記憶
    if (last_triggered_track >= 0 && last_triggered_track < 8) {
      // まだ編集されていない時だけ記憶
      if (last_edited_pitch_track != last_triggered_track) {
        last_edited_pitch_track = last_triggered_track;
      }
    }

    // 編集中の楽器のパラメータを更新
    if (last_edited_pitch_track >= 0 && last_edited_pitch_track < 8) {
      track_pitch[last_edited_pitch_track] = map(current_adc_pitch, 0, 4095, 200, 50) / 100.0f;
    }
    last_adc_pitch = current_adc_pitch;
    last_pitch_edit_time = millis();
  }

  // ADCデバッグ出力（毎フレーム：シリアルモニタで確認）
  static int debug_adc_counter = 0;
  if (debug_adc_counter++ % 10 == 0) {  // 10回に1回に変更（過剰出力防止）
    Serial.print("ADC D:");
    Serial.print(current_adc_decay);
    Serial.print(" P:");
    Serial.print(current_adc_pitch);
    Serial.print(" T:");
    Serial.print(last_edited_decay_track);
    Serial.print("/");
    Serial.print(last_edited_pitch_track);
    Serial.print(" PITCH[");
    for (int t = 0; t < 8; t++) {
      Serial.print(track_pitch[t], 2);
      if (t < 7) Serial.print(" ");
    }
    Serial.println("]");
  }

  // 2. ボタン処理（長押し判定付き）
  bool btn_a = digitalRead(PIN_BTN_A);
  bool btn_b = digitalRead(PIN_BTN_B);

  // ボタンA処理
  if (btn_a == LOW && last_btn_a == HIGH) {
    // ボタンAが押された
    btn_a_state = PRESSING;
    btn_a_press_time = millis();
    last_activity_time = millis();  // ボタン操作でアクティビティ更新
  } else if (btn_a == HIGH && last_btn_a == LOW) {
    // ボタンAが離された
    if (btn_a_state == PRESSING) {
      unsigned long press_duration = millis() - btn_a_press_time;
      if (press_duration < 1000) {
        // SHORT_PRESS: Play/Stopトグル
        is_playing = !is_playing;
        if (is_playing) {
          current_step = 0;
          last_step_time = millis();
        }
      }
    }
    btn_a_state = IDLE;
  }

  // ボタンA長押し検出（1秒経過）
  if (btn_a_state == PRESSING && (millis() - btn_a_press_time >= 1000)) {
    btn_a_state = LONG_PRESSING;
  }

  // ボタンB処理
  if (btn_b == LOW && last_btn_b == HIGH) {
    // ボタンBが押された
    btn_b_state = PRESSING;
    btn_b_press_time = millis();
    last_activity_time = millis();  // ボタン操作でアクティビティ更新
  } else if (btn_b == HIGH && last_btn_b == LOW) {
    // ボタンBが離された
    if (btn_b_state == PRESSING) {
      unsigned long press_duration = millis() - btn_b_press_time;
      if (press_duration < 1000) {
        // SHORT_PRESS: Recordモードトグル
        is_recording = !is_recording;
      }
      // LONG_RELEASE は何もしない
    }
    btn_b_state = IDLE;
  }

  // ボタンB長押し検出（1秒経過）
  if (btn_b_state == PRESSING && (millis() - btn_b_press_time >= 1000)) {
    btn_b_state = LONG_PRESSING;
  }

  last_btn_a = btn_a;
  last_btn_b = btn_b;

  // 3. キーマトリクス スキャン + MIDI統合
  static bool prev_midi_key_state[8] = {false};
  bool key_activity_detected = false;

  // 物理キーマトリクススキャン
  for (int r = 0; r < 2; r++) {
    digitalWrite(ROW_PINS[r], LOW);
    delayMicroseconds(5);

    for (int c = 0; c < 4; c++) {
      int raw_idx = r * 4 + c;
      int key_idx = KEY_MAP[raw_idx];

      bool pressed = (digitalRead(COL_PINS[c]) == LOW);

      if (pressed && !last_key_state[key_idx]) {
        // ボタンB長押し中はノート削除
        if (btn_b_state == LONG_PRESSING) {
          seq[key_idx][current_step] = 0;  // ノート削除
        } else {
          // 通常のキー操作
          if (is_recording) {
            // Recordモード: 現在のステップをトグル
            seq[key_idx][current_step] = !seq[key_idx][current_step];
            // トグルされたら即座に鳴らす
            if (seq[key_idx][current_step]) {
              envs[key_idx] = 1.0;
              phase_reset_request[key_idx] = true;  // Core 1でリセット
              last_triggered_track = key_idx;
              is_manual_trigger = true;
            }
          } else {
            // Playモード: 手動トリガー
            envs[key_idx] = 1.0;
            phase_reset_request[key_idx] = true;  // Core 1でリセット
            last_triggered_track = key_idx;
            is_manual_trigger = true;
          }

          // 【チョーク処理】Close Hat → Open Hat
          if (key_idx == 2) envs[3] = 0;

          // キー操作があったのでアクティビティ時間を更新
          last_activity_time = millis();
          key_activity_detected = true;
        }
      }
      last_key_state[key_idx] = pressed;
    }
    digitalWrite(ROW_PINS[r], HIGH);
  }

  // MIDIキー状態の処理（物理キーと統合）
  for (int i = 0; i < 8; i++) {
    if (g_midi_key_state[i] != prev_midi_key_state[i]) {
      // MIDIキー状態が変化した
      last_key_state[i] = g_midi_key_state[i];

      if (g_midi_key_state[i] && !prev_midi_key_state[i]) {
        // MIDI Note On
        if (is_recording) {
          // Recordモード: 現在のステップをトグル
          seq[i][current_step] = !seq[i][current_step];
          // トグルされたら即座に鳴らす
          if (seq[i][current_step]) {
            envs[i] = 1.0;
            phase_reset_request[i] = true;
            last_triggered_track = i;
            is_manual_trigger = true;
          }
        } else {
          // Playモード: 手動トリガー
          envs[i] = 1.0;
          phase_reset_request[i] = true;
          last_triggered_track = i;
          is_manual_trigger = true;
        }

        // 【チョーク処理】Close Hat → Open Hat
        if (i == 2) envs[3] = 0;

        // MIDI操作があったのでアクティビティ時間を更新
        last_activity_time = millis();
      }
      prev_midi_key_state[i] = g_midi_key_state[i];
    }
  }

  // 4. シーケンサー進行
  int step_delay = (60000 / current_bpm) / 4;
  if (is_playing && (millis() - last_step_time >= step_delay)) {
    last_step_time = millis();

    // メトロノーム: Recordモード時のみ4拍ごとにクリック
    if (is_recording && current_step % 4 == 0) {
      // 1拍目は強く、その他は弱く
      metronome_env = (current_step == 0) ? 1.0 : 0.5;
    }

    // LEDを点灯（Recordモード時は点滅）
    if (current_step % 4 == 0) {
      digitalWrite(PIN_LED, is_recording ? HIGH : HIGH);
      led_on_time = millis();
    }

    // 8トラック全て処理
    for (int t = 0; t < 8; t++) {
      if (seq[t][current_step]) {
        envs[t] = 1.0;
        phase_reset_request[t] = true;  // Core 1でリセット
        // シーケンサー再生時はlast_triggered_trackを更新しない（マニュアル時のみ）
        is_manual_trigger = false;

        // 【チョーク処理】Close Hat(2) → Open Hat(3)
        if (t == 2) envs[3] = 0;
      }
    }
    current_step = (current_step + 1) % 16;
  }

  // LEDの消灯処理 (点灯から30ms経過したら消す)
  if (digitalRead(PIN_LED) == HIGH && (millis() - led_on_time > 30)) {
    digitalWrite(PIN_LED, LOW);
  }

  // 5. OLED 描画
  if (millis() - last_draw_time > 33) {
    drawOLED();
    last_draw_time = millis();
  }
}

// --- OLED描画関数（72x40版） ---
void drawOLED() {
  // アイドル時間をチェック
  unsigned long current_time = millis();
  unsigned long idle_time = current_time - last_activity_time;

  // 3分間操作がない場合、OLEDを消灯
  if (idle_time > OLED_IDLE_TIMEOUT) {
    if (oled_is_on) {
      // 消灯処理
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      oled_is_on = false;

      Serial.println("OLED: Power Save Mode (3 min idle)");
    }
    // 消灯中は描画をスキップ
    return;
  }

  // 操作があった場合、OLEDを再点灯
  if (!oled_is_on) {
    oled_is_on = true;
    Serial.println("OLED: Wake up");
  }

  u8g2.clearBuffer();

  // 編集中のトラック番号表示（最後に編集した方を表示）
  int display_track = -1;
  bool is_decay_edit = false;

  if (last_edited_decay_track >= 0 && last_edited_pitch_track >= 0) {
    // 両方編集されている: 時刻が新しい方
    is_decay_edit = (last_decay_edit_time > last_pitch_edit_time);
    display_track = is_decay_edit ? last_edited_decay_track : last_edited_pitch_track;
  } else if (last_edited_decay_track >= 0) {
    display_track = last_edited_decay_track;
    is_decay_edit = true;
  } else if (last_edited_pitch_track >= 0) {
    display_track = last_edited_pitch_track;
    is_decay_edit = false;
  }

  // 楽器名配列（短縮版）
  const char* INST_NAMES[8] = {"KIK", "SNR", "CH", "OH", "TM1", "TM2", "CLP", "CYM"};

  if (display_track >= 0) {
    // ===== 上部: トラック情報 =====
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(2, 8);
    u8g2.print("T");
    u8g2.print(display_track);
    u8g2.print(" ");
    u8g2.print(INST_NAMES[display_track]);

    // ===== 中央: パラメータ表示 =====
    u8g2.setFont(u8g2_font_7x13B_tr);  // 大きな数字フォント

    // Decay値（左側）
    u8g2.setCursor(2, 24);
    u8g2.print(track_decay[display_track], 2);

    // ラベル（小さなフォント）
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(2, 32);
    u8g2.print("D");
    if (is_decay_edit) {
      u8g2.print("<");  // 編集中マーク
    }

    // Pitch値（右側）
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.setCursor(40, 24);
    u8g2.print(track_pitch[display_track], 2);

    // ラベル（小さなフォント）
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(40, 32);
    u8g2.print("P");
    if (!is_decay_edit) {
      u8g2.print("<");  // 編集中マーク
    }

  } else {
    // まだ編集していない場合
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(10, 15);
    u8g2.print("EDIT A TRACK");
    u8g2.setCursor(15, 30);
    u8g2.print("TO SEE PARAMS");
  }

  // ===== 下部: BPMとモード =====
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(2, 39);
  u8g2.print("BPM:");
  u8g2.print(current_bpm);

  if (is_recording) {
    u8g2.print(" R");
  } else {
    u8g2.print(" P");
  }

  u8g2.sendBuffer();
}

// ==========================================
// Core 1: DSP オーディオエンジン
// ==========================================
float fast_noise() {
  static uint32_t seed = 12345;
  seed = (1103515245 * seed + 12345);
  // / 32768.0f の代わりに掛け算を使う（爆速化）
  return ((float)(seed >> 16) * 0.00003051757f) - 1.0f; 
}

void setup1() {
  i2s.setDATA(PIN_DATA);
  i2s.setBCLK(PIN_LOW);
  i2s.swapClocks();
  i2s.setLSBJFormat();
  i2s.setBitsPerSample(16);
  i2s.begin(SAMPLE_RATE);
}

void loop1() {
  float mix = 0.0f;
  float noise = fast_noise();

  // メトロノーム
  if (metronome_env > 0.001f) {
    metronome_phase += 1200.0f * PHASE_INC_MULT;
    float square = (metronome_phase < PI_F) ? 1.0f : -1.0f;
    if (metronome_phase > TWO_PI_F) metronome_phase -= TWO_PI_F;
    mix += square * metronome_env * 0.8f;
    metronome_env *= 0.99f;
  }

  float local_pitch[8];
  float local_decay[8];
  for (int i = 0; i < 8; i++) {
    local_pitch[i] = track_pitch[i];
    local_decay[i] = track_decay[i];
  }

  for (int i = 0; i < 8; i++) {
    if (phase_reset_request[i]) {
      phases[i] = 0.0f;
      phase_reset_request[i] = false;
    }

    if (envs[i] < 0.001f) continue;

    float out = 0.0f;

    switch (i) {
      case 0:  // Kick
        {
          // 22kHzに合わせてDecay係数を再調整
          const float KICK_BASE = 0.9960f;
          const float KICK_MULT = 0.0038f;
          float decay_factor = KICK_BASE + KICK_MULT * local_decay[i];

          // 割り算を排除！
          phases[i] += (50.0f + 200.0f * envs[i]) * local_pitch[i] * PHASE_INC_MULT;
          if (phases[i] > TWO_PI_F) phases[i] -= TWO_PI_F;

          out = sinf(phases[i]) * envs[i] * 0.8f;
          envs[i] *= decay_factor;
        }
        break;

      case 1:  // Snare
        {
          const float SNARE_BASE = 0.9960f;
          const float SNARE_MULT = 0.0030f;
          float decay_factor = SNARE_BASE + SNARE_MULT * local_decay[i];

          phases[i] += 180.0f * local_pitch[i] * PHASE_INC_MULT;
          if (phases[i] > TWO_PI_F) phases[i] -= TWO_PI_F;

          out = (sinf(phases[i]) * 0.4f + noise * 0.6f) * envs[i] * 0.7f;
          envs[i] *= decay_factor;
        }
        break;

      case 4:
      case 5:  // Tom1 / Tom2（ピューンと下降する音）
        {
          const float TOM_BASE = 0.9960f;
          const float TOM_MULT = 0.0030f;
          float decay_factor = TOM_BASE + TOM_MULT * local_decay[i];

          // ピッチエンベロープ：高い音（150Hz）から低い音（80Hz）へ下降
          float tom_freq = (80.0f + 70.0f * envs[i]) * local_pitch[i];
          phases[i] += tom_freq * PHASE_INC_MULT;
          if (phases[i] > TWO_PI_F) phases[i] -= TWO_PI_F;

          // ノイズ成分を減らしてトーン音を強調（0.4:0.2）
          out = (sinf(phases[i]) * 0.4f + noise * 0.2f) * envs[i] * 0.8f;
          envs[i] *= decay_factor;
        }
        break;

      case 6:  // Clap
        {
          const float CLAP_BASE = 0.9900f;
          const float CLAP_MULT = 0.0080f;
          float decay_factor = CLAP_BASE + CLAP_MULT * local_decay[i];

          float time_ms = phases[i] * 1000.0f;
          float clap_env = 0.0f;

          if (time_ms < 11.0f) {
            clap_env = 1.0f - (time_ms / 11.0f);
          } else if (time_ms < 22.0f) {
            clap_env = 1.0f - ((time_ms - 11.0f) / 11.0f);
          } else if (time_ms < 33.0f) {
            clap_env = 1.0f - ((time_ms - 22.0f) / 11.0f);
          }

          // 余韻（リリース）を長く・大きく
          float tail = 0.0f;
          if (time_ms > 25.0f) {  // 開始を遅く（20ms→25ms）
            // 徐々にフェードイン（0.6→1.0）
            float tail_fade = (time_ms - 25.0f) / 50.0f;  // 50msかけてフェードイン
            if (tail_fade > 1.0f) tail_fade = 1.0f;
            tail = envs[i] * 1.0f * tail_fade;  // 最大値を0.6→1.0に増加
          }

          float total_env = clap_env + tail;

          static float lp1 = 0.0f;
          static float lp2 = 0.0f;

          float color_hi = 0.3f * local_pitch[i];
          if (color_hi > 1.0f) color_hi = 1.0f;
          float color_lo = 0.1f * local_pitch[i];
          if (color_lo > 1.0f) color_lo = 1.0f;

          lp1 += color_hi * (noise - lp1);
          lp2 += color_lo * (lp1 - lp2);
          float filtered_noise = lp1 - lp2;

          out = filtered_noise * total_env * 2.5f;

          // 22kHz用にインクリメント量を変更
          phases[i] += 1.0f / 22050.0f;
          envs[i] *= decay_factor;
        }
        break;

      case 7:  // Cymbal
        {
          const float CYMB_BASE = 0.9990f;
          const float CYMB_MULT = 0.0008f;
          float decay_factor = CYMB_BASE + CYMB_MULT * local_decay[i];

          float metallic = 0.0f;
          float freqs[6] = { 205.3f, 289.4f, 400.0f, 542.2f, 690.3f, 800.1f };

          for (int f = 0; f < 6; f++) {
            cymbal_phases[f] += freqs[f] * local_pitch[i] * PHASE_INC_MULT;
            if (cymbal_phases[f] > TWO_PI_F) cymbal_phases[f] -= TWO_PI_F;

            metallic += (cymbal_phases[f] < PI_F) ? 1.0f : -1.0f;
          }
          metallic /= 6.0f;

          float source = (metallic * 0.7f) + (noise * 0.3f);
          float hp_cymbal = source - cymbal_last_sample;
          cymbal_last_sample = source;

          out = hp_cymbal * (envs[i] * envs[i]) * 0.8f;
          envs[i] *= decay_factor;
        }
        break;

      case 2:
      case 3:  // HiHats (CH/OH)
        {
          const float HH_CLOSE_BASE = 0.9800f;
          const float HH_CLOSE_MULT = 0.0180f;
          const float HH_OPEN_BASE = 0.9980f;
          const float HH_OPEN_MULT = 0.0018f;

          float d = (i == 3) ?
            (HH_OPEN_BASE + HH_OPEN_MULT * local_decay[i]) :
            (HH_CLOSE_BASE + HH_CLOSE_MULT * local_decay[i]);

          // ピッチでハイパスフィルターのカットオフを制御（0.01-0.2の範囲）
          // Pitchが高いほど、より高い周波数を通す（明るい音）
          float hp_coeff = 0.01f + (local_pitch[i] - 0.5f) * 0.19f;
          if (hp_coeff < 0.01f) hp_coeff = 0.01f;
          if (hp_coeff > 0.2f) hp_coeff = 0.2f;

          // ローパスフィルタを使って可変ハイパスを実現
          static float hh_lp[2] = { 0.0f, 0.0f };  // CloseとOpenで独立
          int hh_idx = (i == 3) ? 1 : 0;
          hh_lp[hh_idx] += hp_coeff * (noise - hh_lp[hh_idx]);

          // ハイパス = 原音 - ローパス
          out = (noise - hh_lp[hh_idx]) * envs[i] * 0.6f;
          envs[i] *= d;
        }
        break;
    }
    mix += out;
  }

  hh_last_noise = noise;

  float master_out = mix * 0.35f;

  if (master_out > 1.0f) master_out = 1.0f;
  if (master_out < -1.0f) master_out = -1.0f;

  int16_t sig = (int16_t)(master_out * 32767.0f);
  i2s.write16(sig, sig);
}