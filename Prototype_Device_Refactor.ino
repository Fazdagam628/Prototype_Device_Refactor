// === LIBRARIES ===
#include <Wire.h>
#include <DHT.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === KONSTANTA PIN ===
#define DHT_PIN 3
#define DHT_TYPE DHT22
#define MQ2_PIN A0
#define MQ2_THRESHOLD 100
#define BUZZER_PIN 8
#define LED_PIN 10
#define TRIG_PIN 5
#define ECHO_PIN 6

// === OLED KONFIGURASI ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// === OBJEK ===
DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS3231 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === ICON BITMAP ===
const unsigned char bmp_thermo[] PROGMEM = {
  0x18, 0x3C, 0x3C, 0x24, 0x24, 0x24, 0x3C, 0x3C
};
const unsigned char bmp_humidity[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0x18, 0x00, 0x00
};
const unsigned char bmp_gas[] PROGMEM = {
  0x08, 0x1C, 0x3E, 0x7F, 0x5B, 0x3E, 0x1C, 0x08
};
const unsigned char jarak_bmp[] PROGMEM = {
  0x10,  // 00010000
  0x28,  // 00101000
  0x44,  // 01000100
  0x82,  // 10000010
  0x82,  // 10000010
  0x44,  // 01000100
  0x28,  // 00101000
  0x10   // 00010000
};
const unsigned char jarak_radar_bmp[] PROGMEM = {
  0x18, // 00011000
  0x24, // 00100100
  0x42, // 01000010
  0x81, // 10000001
  0x42, // 01000010
  0x24, // 00100100
  0x18, // 00011000
  0x00  // 00000000
};
const unsigned char jarak_arrow_bmp[] PROGMEM = {
  0x10, // 00010000
  0x30, // 00110000
  0x78, // 01111000
  0xFF, // 11111111
  0x78, // 01111000
  0x30, // 00110000
  0x10, // 00010000
  0x00  // 00000000
};
const unsigned char jarak_wave_bmp[] PROGMEM = {
  0x00, // 00000000
  0x66, // 01100110
  0x99, // 10011001
  0x42, // 01000010
  0x99, // 10011001
  0x66, // 01100110
  0x00, // 00000000
  0x00  // 00000000
};


// === VARIABEL ===
float mq2Value = 0;
bool gasDetected = false;

unsigned long previousMillis = 0;
const long blinkInterval = 200;
bool ledState = false;
bool buzzerState = false;

// === SETUP FUNGSI ===
void setup() {
  Serial.begin(115200);
  pinMode(MQ2_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC dari waktu compile

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED tidak terdeteksi!");
    while (true)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  Serial.println("Sistem Siap. Memanaskan MQ2...");
  // delay(20000);  // Warming up MQ2 sensor
}

// === LOOP UTAMA ===
void loop() {
  readSensors();
  displayData();
  handleGasAlarm();
}

// === SENSOR DAN LOGIKA ===
void readSensors() {
  mq2Value = analogRead(MQ2_PIN);
  gasDetected = mq2Value > MQ2_THRESHOLD;
}

// === FUNGSI ALARM ===
void handleGasAlarm() {
  if (!gasDetected) {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    buzzerState = !buzzerState;

    digitalWrite(LED_PIN, ledState);
    tone(BUZZER_PIN, buzzerState ? 1000 : 1500);
  }
}

// === FUNGSI ULTRASONIK ===
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.0343 / 2;
  return (distance > 2 && distance < 400) ? distance : -1;
}

// === TAMPILAN DISPLAY ===
void displayData() {
  display.clearDisplay();

  if (gasDetected) {
    display.setTextSize(2);
    display.setCursor(20, 10);
    display.println("!!!");
    display.setTextSize(1);
    display.setCursor(10, 32);
    display.println("!! GAS DETECTED !!");
    display.display();
    return;
  }

  displayClock();
  displayDHT();
  displayMQ2();
  displayUltrasonic();
  display.display();
}

void displayClock() {
  DateTime now = rtc.now();
  char dateBuffer[16], timeBuffer[16];
  sprintf(dateBuffer, "%02d/%02d/%04d", now.day(), now.month(), now.year());
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  display.setCursor(0, 0);
  display.print("DATE   : ");
  display.println(dateBuffer);
  display.print("TIME   : ");
  display.println(timeBuffer);
}

void displayDHT() {
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();

  if (isnan(temp) || isnan(humid)) {
    display.setCursor(0, 18);
    display.println("Sensor DHT gagal!");
    return;
  }

  display.drawBitmap(0, 18, bmp_thermo, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 18);
  display.print("TEMP  : ");
  display.print(temp);
  display.println(" C");

  display.drawBitmap(0, 30, bmp_humidity, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 30);
  display.print("HUMID : ");
  display.print(humid);
  display.println(" %");
}

void displayMQ2() {
  display.drawBitmap(0, 42, bmp_gas, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 42);
  display.print("GAS   : ");
  display.println(mq2Value);
}

void displayUltrasonic() {
  float dist = getDistance();
  // display.drawBitmap(0, 54, jarak_bmp, 8, 8, SSD1306_WHITE);
  // display.drawBitmap(0, 54, jarak_radar_bmp, 8, 8, SSD1306_WHITE);
  // display.drawBitmap(0, 54, jarak_arrow_bmp, 8, 8, SSD1306_WHITE);
  display.drawBitmap(0, 54, jarak_wave_bmp, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 54);
  display.print("RANGE : ");
  if (dist != -1) {
    display.print(dist);
    display.println(" Cm");
  } else {
    display.println("Error");
  }
}
