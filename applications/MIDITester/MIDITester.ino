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
const int PIN_LED = 25;  // Pico オンボードLED

// OLED (72x40)
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// --- 統計情報 ---
struct MIDIStats {
  unsigned long usb_message_count;
  unsigned long serial_message_count;
  unsigned long last_usb_time;
  unsigned long last_serial_time;
  unsigned long usb_interval_ms;
  unsigned long serial_interval_ms;

  // 直前のメッセージ情報
  midi::MidiType last_usb_type;
  byte last_usb_data1;
  byte last_usb_data2;

  midi::MidiType last_serial_type;
  byte last_serial_data1;
  byte last_serial_data2;
};

MIDIStats stats = {
  0, 0, 0, 0, 0, 0,
  midi::InvalidType, 0, 0,
  midi::InvalidType, 0, 0
};

// OLED焼きつき防止用：アイドル検出
volatile unsigned long last_activity_time = 0;
const unsigned long OLED_IDLE_TIMEOUT = 180000; // 3分（ミリ秒）
bool oled_is_on = true;

// ==========================================
// MIDIメッセージ文字列変換
// ==========================================
const char* midiTypeToString(midi::MidiType type) {
  switch (type) {
    case midi::NoteOff: return "NoteOff";
    case midi::NoteOn: return "NoteOn";
    case midi::PolyphonicKeyPressure: return "KeyPr";
    case midi::ControlChange: return "CC";
    case midi::ProgramChange: return "PC";
    case midi::ChannelPressure: return "ChPr";
    case midi::PitchBend: return "Bend";
    case midi::SystemExclusive: return "SysEx";
    case midi::TimeCodeQuarterFrame: return "MTC";
    case midi::SongPosition: return "SongPos";
    case midi::SongSelect: return "SongSel";
    case midi::TuneRequest: return "Tune";
    case midi::Clock: return "Clock";
    case midi::Start: return "Start";
    case midi::Continue: return "Cont";
    case midi::Stop: return "Stop";
    case midi::ActiveSensing: return "ActSens";
    case midi::SystemReset: return "Reset";
    default: return "Unknown";
  }
}

// ==========================================
// MIDIメッセージ処理関数
// ==========================================
void processUSBMessage() {
  last_activity_time = millis();

  midi::MidiType type = MIDI_USB.getType();
  byte data1 = MIDI_USB.getData1();
  byte data2 = MIDI_USB.getData2();
  byte channel = MIDI_USB.getChannel();

  stats.usb_message_count++;

  unsigned long current_time = millis();
  if (stats.last_usb_time > 0) {
    stats.usb_interval_ms = current_time - stats.last_usb_time;
  }
  stats.last_usb_time = current_time;

  stats.last_usb_type = type;
  stats.last_usb_data1 = data1;
  stats.last_usb_data2 = data2;

  // 詳細ログ
  Serial.print("[USB] #");
  Serial.print(stats.usb_message_count);
  Serial.print(" Ch:");
  Serial.print(channel);
  Serial.print(" ");
  Serial.print(midiTypeToString(type));
  Serial.print(" ");
  Serial.print(data1);
  Serial.print(" ");
  Serial.print(data2);
  Serial.print(" (");
  Serial.print(stats.usb_interval_ms);
  Serial.println("ms)");

  // LED フラッシュ
  digitalWrite(PIN_LED, HIGH);
  delayMicroseconds(1000);
  digitalWrite(PIN_LED, LOW);
}

void processSerialMessage() {
  last_activity_time = millis();

  midi::MidiType type = MIDI_SERIAL.getType();
  byte data1 = MIDI_SERIAL.getData1();
  byte data2 = MIDI_SERIAL.getData2();
  byte channel = MIDI_SERIAL.getChannel();

  stats.serial_message_count++;

  unsigned long current_time = millis();
  if (stats.last_serial_time > 0) {
    stats.serial_interval_ms = current_time - stats.last_serial_time;
  }
  stats.last_serial_time = current_time;

  stats.last_serial_type = type;
  stats.last_serial_data1 = data1;
  stats.last_serial_data2 = data2;

  // 詳細ログ
  Serial.print("[SERIAL] #");
  Serial.print(stats.serial_message_count);
  Serial.print(" Ch:");
  Serial.print(channel);
  Serial.print(" ");
  Serial.print(midiTypeToString(type));
  Serial.print(" ");
  Serial.print(data1);
  Serial.print(" ");
  Serial.print(data2);
  Serial.print(" (");
  Serial.print(stats.serial_interval_ms);
  Serial.println("ms)");

  // LED フラッシュ（長め）
  digitalWrite(PIN_LED, HIGH);
  delayMicroseconds(2000);
  digitalWrite(PIN_LED, LOW);
}

// ==========================================
// OLED描画関数
// ==========================================
void drawOLED() {
  // アイドル時間をチェック
  unsigned long current_time = millis();
  unsigned long idle_time = current_time - last_activity_time;

  // 3分間操作がない場合、OLEDを消灯
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

  // ===== 上部: メッセージカウンター =====
  u8g2.setFont(u8g2_font_5x7_tr);

  u8g2.setCursor(0, 7);
  u8g2.print("U:");
  u8g2.print(stats.usb_message_count);

  u8g2.setCursor(40, 7);
  u8g2.print("S:");
  u8g2.print(stats.serial_message_count);

  // ===== 中央: 直前のメッセージ =====
  u8g2.setCursor(0, 17);
  u8g2.print("USB:");
  u8g2.print(midiTypeToString(stats.last_usb_type));
  u8g2.print(" ");
  u8g2.print(stats.last_usb_data1);

  if (stats.last_usb_data2 > 0) {
    u8g2.print(" v");
    u8g2.print(stats.last_usb_data2);
  }

  u8g2.setCursor(0, 26);
  u8g2.print("SR:");
  u8g2.print(midiTypeToString(stats.last_serial_type));
  u8g2.print(" ");
  u8g2.print(stats.last_serial_data1);

  if (stats.last_serial_data2 > 0) {
    u8g2.print(" v");
    u8g2.print(stats.last_serial_data2);
  }

  // ===== 下部: インターバル =====
  u8g2.setCursor(0, 36);
  u8g2.print("U:");
  u8g2.print(stats.usb_interval_ms);
  u8g2.print("ms");

  u8g2.setCursor(40, 36);
  u8g2.print("S:");
  u8g2.print(stats.serial_interval_ms);
  u8g2.print("ms");

  u8g2.sendBuffer();
}

// ==========================================
// セットアップ
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Wire.setSDA(4);
  Wire.setSCL(5);
  u8g2.begin();

  last_activity_time = millis();

  Serial.println("========================================");
  Serial.println("MIDITester - USB + Serial MIDI Monitor");
  Serial.println("========================================");
  Serial.println("Monitoring both USB and DIN MIDI...");
  Serial.println("Connect your MIDI controller and play!");
  Serial.println("");
}

// ==========================================
// メインループ
// ==========================================
void loop() {
  // USB MIDI ポーリング
  if (MIDI_USB.read()) {
    processUSBMessage();
  }

  // シリアル MIDI ポーリング
  if (MIDI_SERIAL.read()) {
    processSerialMessage();
  }

  // OLED 更新（33msごと）
  static unsigned long last_draw_time = 0;
  if (millis() - last_draw_time > 33) {
    drawOLED();
    last_draw_time = millis();
  }
}
