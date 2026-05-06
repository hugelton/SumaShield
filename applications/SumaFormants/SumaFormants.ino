#include <I2S.h>
#include <Wire.h>
#include <U8g2lib.h>

// Version information
#define SUMAFORMANTS_VERSION_MAJOR 1
#define SUMAFORMANTS_VERSION_MINOR 0
#define SUMAFORMANTS_VERSION_PATCH 0
#define SUMAFORMANTS_VERSION "1.0.0"

// USB MIDI対応（TinyUSBスタック使用）
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// MIDIインスタンス作成（USB + シリアル）
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);

// UART0でレガシーMIDI DIN（GPIO0=TX, GPIO1=RX）
#define MIDI_SERIAL_PORT Serial1
MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL_PORT, MIDI_SERIAL);

// ==========================================
// --- ピン設定 (ハードウェアはそのまま) ---
// ==========================================
const int PIN_DATA = 19;
const int PIN_LOW  = 20;

const int PIN_DECAY_ADC = 26; // ブレス量 (Breath Mix)
const int PIN_PITCH_ADC = 27; // ピッチ (Pitch)
const int PIN_BTN_A     = 7;  // オクターブ・ドロップ
const int PIN_BTN_B     = 8;  // (予備)

const int PIN_ROTARY_SW  = 9;
const int PIN_ROTARY_CLK = 10;
const int PIN_ROTARY_DT  = 11;

const int COL_PINS[4] = { 12, 13, 14, 15 };
const int ROW_PINS[2] = { 16, 17 };
const int KEY_MAP[8]  = { 0, 4, 1, 5, 2, 6, 3, 7 };

// モノフォニック用：最後に押されたキーを追跡
volatile int last_active_key = -1;

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// ==========================================
// --- フォルマント定義 (IPA 8基本母音) ---
// ==========================================
const char* VOWEL_NAMES[8] = {"[i]", "[e]", "[E]", "[a]", "[A]", "[C]", "[o]", "[u]"};

// 各母音の F1, F2, F3 周波数
const float VOWEL_F1[8] = { 240.0f, 390.0f, 610.0f, 850.0f, 750.0f, 500.0f, 360.0f, 250.0f };
const float VOWEL_F2[8] = { 2400.0f, 2300.0f, 1900.0f, 1610.0f, 1090.0f, 700.0f, 640.0f, 595.0f };
const float VOWEL_F3[8] = { 3200.0f, 3000.0f, 2800.0f, 2500.0f, 2440.0f, 2300.0f, 2250.0f, 2110.0f };

// ==========================================
// --- グローバル通信用パラメータ (Core 0 <-> Core 1) ---
// ==========================================
volatile bool  g_key_state[8]  = { false };
volatile float g_breath        = 0.0f;   // 0.0 ~ 1.0
volatile float g_pitch_semitone = 0.0f;  // ピッチ（セミトーン単位、-12 ~ +12）
volatile float g_formant_offset = 0.0f; // フォルマントオフセット（Hz単位、-100 ~ +100）
volatile bool  g_octave_down   = false;
volatile float g_envs[8]       = { 0.0f }; // Core 1が更新、Core 0が参照
volatile int  g_key_change_flag = -1;    // キー切り替えフラグ（切り替わった瞬間に新しいキー番号をセット）

// MIDI用グローバル変数
volatile bool  g_midi_key_state[8] = { false };  // MIDIから来るキー状態
volatile int   g_midi_note_map[8] = {60, 62, 64, 65, 67, 69, 71, 72}; // MIDIノート番号マップ（C4-D5）
volatile bool  g_midi_active = false;          // MIDI入力が有効かどうか
volatile float g_midi_pitch = 0.0f;            // MIDIノートから計算したピッチ（0ならMIDI未使用）

volatile int rotary_counter = 0;
volatile bool is_recording   = false; // （将来の録音機能用）

// OLED焼きつき防止用：アイドル検出
volatile unsigned long last_activity_time = 0;
const unsigned long OLED_IDLE_TIMEOUT = 180000; // 3分（ミリ秒）
bool oled_is_on = true;

// ピッチ基本値（ADCで制御）
volatile float g_base_pitch = 220.0f; // デフォルトA3 (ラ)

// MIDIメッセージ処理関数（共通）
void processMIDIMessage(midi::MidiType type, byte note, byte velocity) {
  if (type == midi::NoteOn) {
    if (velocity > 0) {
      // Note On: ノート番号を母音キーにマッピング
      for (int i = 0; i < 8; i++) {
        if (note == g_midi_note_map[i]) {
          g_midi_key_state[i] = true;
          g_midi_active = true;
          break;
        }
      }

      // ノート番号からピッチを計算（A4=69=440Hz）
      // freq = 440 * 2^((note - 69) / 12)
      g_midi_pitch = 440.0f * powf(2.0f, (float)(note - 69) / 12.0f);
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
    // Note Off
    for (int i = 0; i < 8; i++) {
      if (note == g_midi_note_map[i]) {
        g_midi_key_state[i] = false;
        break;
      }
    }
  }
}

I2S i2s(OUTPUT);
const int SAMPLE_RATE = 22050; // 軽量＆Lo-Fi

// ==========================================
// --- Core 0: UI & コントロール ---
// ==========================================
void rotary_isr() {
  // デバウンス処理（チャタリング防止）
  static unsigned long last_irq = 0;
  static uint8_t last_clk = HIGH;

  // 前回の割り込みから10ms以上経過しているか確認
  if (millis() - last_irq < 10) {
    return;
  }

  // CLKピンの状態を読み取り
  uint8_t current_clk = digitalRead(PIN_ROTARY_CLK);

  // 立ち下がりエッジ検出（HIGH → LOW）
  if (last_clk == HIGH && current_clk == LOW) {
    // DTピンの状態で回転方向を判定
    uint8_t dt_state = digitalRead(PIN_ROTARY_DT);

    if (dt_state == LOW) {
      rotary_counter--;  // 右回転：フォルマントを高く
    } else {
      rotary_counter++;  // 左回転：フォルマントを低く
    }

    // 制限: -100Hz 〜 +100Hz
    if (rotary_counter < -100) rotary_counter = -100;
    if (rotary_counter > 100) rotary_counter = 100;

    // フォルマントオフセットとして設定
    g_formant_offset = (float)rotary_counter;

    // ロータリ操作があったのでアクティビティ時間を更新
    last_activity_time = millis();
  }

  last_clk = current_clk;
  last_irq = millis();
}

// --- OLED描画関数（フォルマント・フェイス版） ---
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

  // 1. 現在鳴っている音の「最大音量」と「どの母音が鳴っているか」を取得
  float max_env = 0.0f;
  int active_vowel = -1;

  // 最後に押されたキーを優先（last_active_keyを使用）
  if (last_active_key >= 0) {
    active_vowel = last_active_key;
    max_env = g_envs[last_active_key];
  } else {
    // キーが離されている場合、エンベロープが大きいものを選択
    for (int i = 0; i < 8; i++) {
      if (g_envs[i] > max_env) {
        max_env = g_envs[i];
        active_vowel = i;
      }
    }
  }

  // 2. まばたきの自動計算（3〜5秒に1回、一瞬だけ目を閉じる）
  static unsigned long last_blink_time = 0;
  static bool is_blinking = false;
  if (!is_blinking && millis() - last_blink_time > 3000 + random(2000)) {
    is_blinking = true;
    last_blink_time = millis();
  }
  if (is_blinking && millis() - last_blink_time > 150) { // まばたきの速度(150ms)
    is_blinking = false;
  }

  // 3. 顔の描画パラメータ (画面サイズ: 72 x 40)
  int center_x = 36;
  int center_y = 20;

  // --- 目の描画 ---
  int eye_w = 6;
  int eye_h = is_blinking ? 2 : 8; // まばたき中は糸目になる
  int eye_offset_x = 14;           // 目と目の離れ具合
  int eye_y = 12;

  // 左目と右目
  u8g2.drawBox(center_x - eye_offset_x - eye_w/2, eye_y - eye_h/2, eye_w, eye_h);
  u8g2.drawBox(center_x + eye_offset_x - eye_w/2, eye_y - eye_h/2, eye_w, eye_h);

  // --- 口の描画 ---
  int mouth_y = 28;
  int mouth_w = 8;
  int mouth_h = 2; // デフォルトは閉じた口

  // 音が鳴っている（喋っている）とき
  if (max_env > 0.02f && active_vowel >= 0) {
    // 母音(active_vowel)によって口の「横幅」と「開き方」を変えるギミック
    if (active_vowel == 0 || active_vowel == 1) {
      // 0:[i], 1:[e] -> 「い」「え」は口を横に「にっ」と引く
      mouth_w = 20;
      // 音量の半分だけ縦に開く（薄く開く）
      mouth_h = 2 + (int)(max_env * 6.0f);
    } else if (active_vowel == 6 || active_vowel == 7) {
      // 6:[o], 7:[u] -> 「お」「う」は口を「すぼめる」
      mouth_w = 6;
      mouth_h = 2 + (int)(max_env * 12.0f);
    } else {
      // 2:[ɛ], 3:[a], 4:[ɑ], 5:[ɔ] -> 「あ」などは大きく開ける
      mouth_w = 12;
      mouth_h = 2 + (int)(max_env * 12.0f);
    }

    // 最大値制限
    if (mouth_h > 14) mouth_h = 14;
  }

  // 口の描画（角丸の四角形にして柔らかい表情に）
  u8g2.drawRBox(center_x - mouth_w/2, mouth_y - mouth_h/2, mouth_w, mouth_h, 1);

  // フォルマントオフセットインジケーター（左上）
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(0, 6);
  int offset_hz = (int)g_formant_offset;
  if (offset_hz >= 0) {
    u8g2.print("+");
    u8g2.print(offset_hz);
  } else {
    u8g2.print(offset_hz);
  }
  u8g2.print("Hz");
  u8g2.print(" ");
  u8g2.print((int)g_base_pitch);
  u8g2.print("Hz");

  // ブレスインジケーター（右上）
  u8g2.setCursor(56, 6);
  int breath_pct = (int)(g_breath * 100);
  u8g2.print(breath_pct);
  u8g2.print("%");

  // （おまけ）Recordモードのときは、画面右上に小さな [REC] マークを表示
  if (is_recording) {
    u8g2.setCursor(50, 40);
    u8g2.print("R");
  }

  // MIDIアクティブインジケーター（下部）
  if (g_midi_active) {
    u8g2.setCursor(30, 40);
    u8g2.print("MIDI");
  }

  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  Serial.print("SumaFormants v");
  Serial.println(SUMAFORMANTS_VERSION);
  Serial.println("Monophonic formant vocoder synth with 8 vowel sounds");
  Serial.println();

  // TinyUSBスタック初期化
  TinyUSB_Device_Init(0);

  // USB MIDI初期化
  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  MIDI_USB.turnThruOff(); // スルー機能をオフ

  // UART0でレガシーMIDI DIN初期化（GPIO0=TX, GPIO1=RX, 31250bps）
  MIDI_SERIAL_PORT.setTX(0);
  MIDI_SERIAL_PORT.setRX(1);
  MIDI_SERIAL.begin(MIDI_CHANNEL_OMNI);
  MIDI_SERIAL.turnThruOff();

  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
  pinMode(PIN_ROTARY_SW, INPUT_PULLUP);
  pinMode(PIN_ROTARY_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_DT, INPUT_PULLUP);

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

  // 初期化時にアクティビティ時間をセット
  last_activity_time = millis();

  Serial.println("SumaFormant with USB MIDI + Serial MIDI - Ready!");
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

  // 1. ADC読み取りとスムージング
  static float smooth_adc_pitch = 2048.0f;
  static float smooth_adc_breath = 0.0f;
  static float prev_smooth_adc_pitch = 0.0f;
  static float prev_smooth_adc_breath = 0.0f;

  float raw_pitch = 4095.0f - analogRead(PIN_PITCH_ADC);     // 反転：ピッチ
  float raw_breath = 4095.0f - analogRead(PIN_DECAY_ADC);    // 反転：ブレス量

  smooth_adc_pitch += 0.1f * (raw_pitch - smooth_adc_pitch);
  smooth_adc_breath += 0.1f * (raw_breath - smooth_adc_breath);

  // ADC変化検出（閾値：10単位以上で操作とみなす）
  if (fabsf(smooth_adc_pitch - prev_smooth_adc_pitch) > 10.0f ||
      fabsf(smooth_adc_breath - prev_smooth_adc_breath) > 10.0f) {
    last_activity_time = millis();
  }
  prev_smooth_adc_pitch = smooth_adc_pitch;
  prev_smooth_adc_breath = smooth_adc_breath;

  // ピッチ（指数関数的に割り当て 50Hz 〜 1000Hz）
  float normalized_pitch = smooth_adc_pitch / 4095.0f;
  float adc_base_pitch = 50.0f * expf(normalized_pitch * 3.0f);

  // MIDIピッチが有効ならMIDIを優先、そうでなければADCを使用
  if (g_midi_pitch > 0.0f) {
    g_base_pitch = g_midi_pitch;
  } else {
    g_base_pitch = adc_base_pitch;
  }

  // ブレス量は直線補間 (0.0 〜 1.0)
  g_breath = smooth_adc_breath / 4095.0f;

  // 2. オクターブダウンボタン
  static bool prev_octave_down = false;
  bool current_octave_down = (digitalRead(PIN_BTN_A) == LOW);
  g_octave_down = current_octave_down;

  // ボタン変化検出
  if (current_octave_down != prev_octave_down) {
    last_activity_time = millis();
  }
  prev_octave_down = current_octave_down;

  // 3. マトリクススキャン + MIDI統合（モノフォニック：最後に押されたキーのみ発音）
  static unsigned long last_log_time = 0;
  static bool prev_key_state[8] = {false};
  static bool prev_midi_key_state[8] = {false};
  static int prev_last_active_key = -1;
  bool key_activity_detected = false;

  // 物理キーマトリクススキャン
  bool physical_key_pressed = false;
  for (int r = 0; r < 2; r++) {
    digitalWrite(ROW_PINS[r], LOW);
    delayMicroseconds(5);

    for (int c = 0; c < 4; c++) {
      int raw_idx = r * 4 + c;
      int key_idx = KEY_MAP[raw_idx];
      bool key_pressed = (digitalRead(COL_PINS[c]) == LOW);

      // キー状態を更新
      if (key_pressed != g_key_state[key_idx]) {
        g_key_state[key_idx] = key_pressed;
        key_activity_detected = true;

        // キーが押された場合、最後のアクティブキーを更新
        if (key_pressed) {
          last_active_key = key_idx;
          // 物理キーが押されたらMIDIピッチをクリア（ADC制御に戻す）
          g_midi_pitch = 0.0f;
        }
      }

      // 前回の状態を保存
      prev_key_state[key_idx] = g_key_state[key_idx];

      // 物理キーが押されているかチェック
      if (g_key_state[key_idx]) {
        physical_key_pressed = true;
      }
    }
    digitalWrite(ROW_PINS[r], HIGH);
  }

  // MIDIキーも統合して処理
  for (int i = 0; i < 8; i++) {
    if (g_midi_key_state[i] != prev_midi_key_state[i]) {
      key_activity_detected = true;

      // MIDIキーが押された場合、最後のアクティブキーを更新
      if (g_midi_key_state[i]) {
        last_active_key = i;
      }
    }
    prev_midi_key_state[i] = g_midi_key_state[i];

    // 物理キーとMIDIキーのORをとる（どちらかが押されていれば発音）
    if (g_midi_key_state[i] || g_key_state[i]) {
      // MIDIがアクティブなら、物理キー状態を上書き
      if (g_midi_key_state[i]) {
        g_key_state[i] = true;
      }
    }
  }

  // キー操作があった場合、アクティビティ時間を更新
  if (key_activity_detected) {
    last_activity_time = millis();
  }

  // キー切り替え検出（前回と異なるキーが押された場合）
  if (last_active_key != prev_last_active_key && last_active_key >= 0) {
    g_key_change_flag = last_active_key; // 新しいキー番号をセット
  }
  prev_last_active_key = last_active_key;

  // 全キーが離された場合、アクティブキーをリセット
  bool any_key_pressed = false;
  for (int i = 0; i < 8; i++) {
    if (g_key_state[i]) {
      any_key_pressed = true;
      break;
    }
  }
  if (!any_key_pressed) {
    last_active_key = -1;
  }

  // 4. Serialログ（1秒ごとに現在のvowelを出力）
  if (millis() - last_log_time > 1000) {
    if (last_active_key >= 0 && g_envs[last_active_key] > 0.01f) {
      Serial.print("Active Vowel: ");
      Serial.print(VOWEL_NAMES[last_active_key]);
      Serial.print(" (key ");
      Serial.print(last_active_key);
      Serial.print(") env=");
      Serial.print(g_envs[last_active_key]);
      Serial.print(" pitch=");
      Serial.print(g_pitch_semitone);
      Serial.print("st base=");
      Serial.print((int)g_base_pitch);
      Serial.print("Hz breath=");
      Serial.print(g_breath);
      Serial.print(" midi=");
      if (g_midi_pitch > 0.0f) {
        Serial.print("PITCH ");
        Serial.println((int)g_midi_pitch);
      } else {
        Serial.println("OFF");
      }
    }
    last_log_time = millis();
  }

  // 5. OLED表示 (33msごとに更新) - 顔アニメーション版
  static unsigned long last_draw = 0;
  if (millis() - last_draw > 33) {
    drawOLED();
    last_draw = millis();
  }
}

// ==========================================
// --- Core 1: DSP オーディオエンジン ---
// ==========================================
const float PI_F = 3.14159265f;
const float TWO_PI_F = 6.2831853f;
const float PHASE_INC_MULT = TWO_PI_F / 22050.0f;

float fast_noise() {
  static uint32_t seed = 12345;
  seed = (1103515245 * seed + 12345);
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
  static float phase = 0;

  // SVF用メモリ（モノフォニックなので1つだけでOK）
  static float bp1=0, lp1=0;
  static float bp2=0, lp2=0;
  static float bp3=0, lp3=0;

  // フォルマントポートメント用：現在のF1/F2/F3値と前回のアクティブキー
  static float current_f1 = 240.0f;
  static float current_f2 = 2400.0f;
  static float current_f3 = 3200.0f;
  static int prev_active_key = -1;

  // ポートメント制御用
  static float portamento_counter = 0.0f;
  const float PORTAMENTO_DURATION = 0.05f; // キー切り替え時のポートメント長さ（秒）

  float mix = 0.0f;
  float noise = fast_noise();

  // Core 0からの値を安全に取得
  float pitch_semitone = g_pitch_semitone;
  if (g_octave_down) pitch_semitone -= 12.0f; // オクターブ下

  // セミトーンから周波数へ変換: freq = base * 2^(semitone/12)
  float current_pitch = g_base_pitch * powf(2.0f, pitch_semitone / 12.0f);

  float current_breath = g_breath;
  int active_key = last_active_key;
  int key_change = g_key_change_flag;

  // キー切り替え検出時、ポートメントカウンターをリセット
  if (key_change >= 0) {
    portamento_counter = PORTAMENTO_DURATION;
    g_key_change_flag = -1; // フラグをクリア
  }

  // アクティブキーが変わった場合、フォルマント値をターゲットにリセット（ズレ防止）
  if (active_key != prev_active_key) {
    if (active_key >= 0) {
      // ロータリーエンコーダーによるオフセット適用
      current_f1 = VOWEL_F1[active_key] + g_formant_offset;
      current_f2 = VOWEL_F2[active_key] + g_formant_offset;
      current_f3 = VOWEL_F3[active_key] + g_formant_offset;

      // フォルマント周波数が最低100Hzを下回らないようにクランプ
      if (current_f1 < 100.0f) current_f1 = 100.0f;
      if (current_f2 < 100.0f) current_f2 = 100.0f;
      if (current_f3 < 100.0f) current_f3 = 100.0f;
    }
    prev_active_key = active_key;
  }

  // 全エンベロープを更新（リリース処理）
  for (int i = 0; i < 8; i++) {
    if (i == active_key && g_key_state[i]) {
      g_envs[i] += 0.02f; // アタック
      if (g_envs[i] > 1.0f) g_envs[i] = 1.0f;
    } else {
      g_envs[i] *= 0.985f; // リリース（少し速く）
    }

    if (g_envs[i] < 0.001f) {
      g_envs[i] = 0.0f;
    }
  }

  // アクティブキーがあれば発音
  if (active_key >= 0 && g_envs[active_key] > 0.001f) {
    int i = active_key;

    // --- 音源生成 (ノコギリ波 + ノイズ) ---
    phase += current_pitch * PHASE_INC_MULT;
    if (phase > TWO_PI_F) phase -= TWO_PI_F;

    float saw = (phase / PI_F) - 1.0f;
    float source = (saw * (1.0f - current_breath)) + (noise * current_breath);

    // --- フォルマントポートメント ---
    // ターゲット値を設定（ロータリーエンコーダーによるオフセット適用）
    float target_f1 = VOWEL_F1[i] + g_formant_offset;
    float target_f2 = VOWEL_F2[i] + g_formant_offset;
    float target_f3 = VOWEL_F3[i] + g_formant_offset;

    // フォルマント周波数が最低100Hzを下回らないようにクランプ
    if (target_f1 < 100.0f) target_f1 = 100.0f;
    if (target_f2 < 100.0f) target_f2 = 100.0f;
    if (target_f3 < 100.0f) target_f3 = 100.0f;

    if (portamento_counter > 0.0f) {
      // キー切り替え直後：ポートメント有効
      portamento_counter -= (1.0f / 22050.0f);

      // ポートメント速度：0.05秒で遷移
      float portamento_speed = (1.0f / 22050.0f) / PORTAMENTO_DURATION;

      // 現在値をターゲットに向かって移動
      current_f1 += (target_f1 - current_f1) * portamento_speed;
      current_f2 += (target_f2 - current_f2) * portamento_speed;
      current_f3 += (target_f3 - current_f3) * portamento_speed;

      // ターゲットに十分近づいたら、強制的にターゲット値に固定
      if (fabsf(target_f1 - current_f1) < 0.5f) current_f1 = target_f1;
      if (fabsf(target_f2 - current_f2) < 2.0f) current_f2 = target_f2;
      if (fabsf(target_f3 - current_f3) < 2.0f) current_f3 = target_f3;
    } else {
      // ポートメント終了：ターゲット値に固定
      current_f1 = target_f1;
      current_f2 = target_f2;
      current_f3 = target_f3;
    }

    // --- フォルマント・フィルタ (Chamberlin SVF) ---
    float q = 0.15f; // レゾナンス（声の鋭さ）

    // F1 フィルタ
    float f1_coef = TWO_PI_F * current_f1 / 22050.0f;
    if(f1_coef > 0.85f) f1_coef = 0.85f; // 安全マージン増
    bp1 += f1_coef * (source - lp1 - q * bp1);
    lp1 += f1_coef * bp1;

    // F2 フィルタ
    float f2_coef = TWO_PI_F * current_f2 / 22050.0f;
    if(f2_coef > 0.85f) f2_coef = 0.85f;
    bp2 += f2_coef * (source - lp2 - q * bp2);
    lp2 += f2_coef * bp2;

    // F3 フィルタ
    float f3_coef = TWO_PI_F * current_f3 / 22050.0f;
    if(f3_coef > 0.85f) f3_coef = 0.85f;
    bp3 += f3_coef * (source - lp3 - q * bp3);
    lp3 += f3_coef * bp3;

    // 3帯域のミックス
    float vowel = bp1 + (bp2 * 0.7f) + (bp3 * 0.4f);
    mix = vowel * g_envs[i];
  } else {
    // 発音していない場合はフィルタ状態をリセット（DCオフセット除去）
    bp1 = 0; lp1 = 0;
    bp2 = 0; lp2 = 0;
    bp3 = 0; lp3 = 0;
  }

  // --- マスター出力 ---
  float master_out = mix * 0.8f;
  if (master_out > 1.0f) master_out = 1.0f;
  if (master_out < -1.0f) master_out = -1.0f;

  int16_t sig = (int16_t)(master_out * 32767.0f);
  i2s.write16(sig, sig);
}
