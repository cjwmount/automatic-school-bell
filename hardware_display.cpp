/*
 * Automated School Bell System - Hardware & Display Module
 * Person 1: Hardware & Display Engineer
 * Handles: ST7735S Display, LEDs, Buzzer
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ==================== PIN CONFIGURATION ====================
#define TFT_CS     5
#define TFT_RST    16
#define TFT_DC     17
#define TFT_MOSI   23
#define TFT_SCLK   18

#define BUZZER_PIN 25

#define LED_GREEN  26
#define LED_RED    27
#define LED_BLUE   14
#define LED_WHITE  12

// ==================== DISPLAY DIMENSIONS ====================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// ==================== GLOBAL VARIABLES (Shared) ====================
extern int eventCount;
extern bool isRinging;
extern int lastSecond;
extern int lastMinute;
extern int lastEventCount;
extern String lastNextBell;
extern bool lastWifiStatus;
extern int lastDay;
extern String lastTimeStr;

// ==================== DISPLAY OBJECT ====================
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ==================== LED FUNCTIONS ====================
void initLEDs() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_WHITE, OUTPUT);
  
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_WHITE, LOW);
}

void setLED(int green, int red, int blue, int white) {
  digitalWrite(LED_GREEN, green);
  digitalWrite(LED_RED, red);
  digitalWrite(LED_BLUE, blue);
  digitalWrite(LED_WHITE, white);
}

void allLEDsOff() {
  setLED(LOW, LOW, LOW, LOW);
}

// ==================== DISPLAY FUNCTIONS ====================
void initDisplay() {
  Serial.println("Initializing ST7735S display (128x160)...");
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 50);
  tft.print("School");
  tft.setCursor(20, 75);
  tft.print("Bell Sys");
  tft.setTextSize(1);
  tft.setCursor(30, 110);
  tft.print("Starting...");
  Serial.println("Display OK");
  delay(1500);
  tft.fillScreen(ST77XX_BLACK);
  drawStaticElements();
}

void drawStaticElements() {
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 5);
  tft.print("SCHOOL BELL SYSTEM");
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 65);
  tft.print("Next:");
  
  tft.setCursor(5, 85);
  tft.print("Events:");
  
  tft.setCursor(5, 105);
  tft.print("Date:");
  
  tft.setCursor(5, 125);
  tft.print("WiFi:");
  
  tft.drawLine(0, 145, 160, 145, ST77XX_BLUE);
}

void updateDisplay(DateTime now, String nextBell, bool wifiConnected) {
  // Update time
  char timeBuf[20];
  snprintf(timeBuf, 20, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  String currentTimeStr = String(timeBuf);
  
  if (currentTimeStr != lastTimeStr) {
    tft.fillRect(5, 30, 150, 30, ST77XX_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(5, 30);
    tft.print(currentTimeStr);
    lastTimeStr = currentTimeStr;
    lastSecond = now.second();
  }
  
  // Update next bell
  if (now.minute() != lastMinute) {
    if (nextBell != lastNextBell) {
      tft.fillRect(45, 65, 100, 15, ST77XX_BLACK);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setCursor(45, 65);
      tft.print(nextBell);
      lastNextBell = nextBell;
    }
    lastMinute = now.minute();
  }
  
  // Update event count
  if (eventCount != lastEventCount) {
    tft.fillRect(60, 85, 80, 15, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(60, 85);
    tft.print(eventCount);
    lastEventCount = eventCount;
  }
  
  // Update date
  if (now.day() != lastDay) {
    tft.fillRect(45, 105, 100, 15, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(45, 105);
    char dateBuf[20];
    snprintf(dateBuf, 20, "%02d/%02d/%04d", now.day(), now.month(), now.year());
    tft.print(dateBuf);
    lastDay = now.day();
  }
  
  // Update WiFi status
  if (wifiConnected != lastWifiStatus) {
    tft.fillRect(45, 125, 100, 15, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(45, 125);
    if (wifiConnected) {
      tft.setTextColor(ST77XX_GREEN);
      tft.print("Connected");
    } else {
      tft.setTextColor(ST77XX_RED);
      tft.print("Disconnected");
    }
    lastWifiStatus = wifiConnected;
  }
  
  // Update ringing status
  static bool lastRinging = false;
  if (isRinging != lastRinging) {
    tft.fillRect(5, 145, 150, 15, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 145);
    if (isRinging) {
      tft.setTextColor(ST77XX_RED);
      tft.print(">>> BELL RINGING! <<<");
    }
    lastRinging = isRinging;
  }
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
}

// ==================== BUZZER FUNCTIONS ====================
void ringBell(int duration) {
  if (isRinging) return;
  
  isRinging = true;
  Serial.printf("BELL RINGING for %d seconds\n", duration);
  
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_WHITE, HIGH);
  
  unsigned long startTime = millis();
  while (millis() - startTime < (duration * 1000)) {
    tone(BUZZER_PIN, 2000);
    delay(80);
    noTone(BUZZER_PIN);
    delay(20);
  }
  
  noTone(BUZZER_PIN);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_WHITE, LOW);
  
  isRinging = false;
  Serial.println("Bell stopped");
}

// ==================== HEARTBEAT LED ====================
void heartbeatLED() {
  static unsigned long lastHeartbeat = 0;
  static bool heartbeatState = false;
  
  if (millis() - lastHeartbeat >= 5000 && !isRinging) {
    heartbeatState = !heartbeatState;
    digitalWrite(LED_GREEN, heartbeatState ? HIGH : LOW);
    lastHeartbeat = millis();
  }
}
