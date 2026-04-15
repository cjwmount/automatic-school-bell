/*
 * Automated School Bell System - ST7735S Optimized
 * Resolution: 128x160 pixels
 * Fixed display fitting and refresh issues
 */

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <ArduinoJson.h>

// ==================== PIN CONFIGURATION ====================
#define TFT_CS     5
#define TFT_RST    16
#define TFT_DC     17
#define TFT_MOSI   23
#define TFT_SCLK   18

#define RTC_SDA    21
#define RTC_SCL    22

#define BUZZER_PIN 25

#define LED_GREEN  26
#define LED_RED    27
#define LED_BLUE   14
#define LED_WHITE  12

// ==================== DISPLAY DIMENSIONS ====================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// ==================== WIFI CONFIGURATION ====================
const char* ssid = "Capital";        // CHANGE THIS
const char* password = "@Bravin20..0"; // CHANGE THIS

// ==================== SYSTEM CONFIGURATION ====================
#define EEPROM_SIZE 512
#define MAGIC_NUMBER 0xBE11
#define SCHEDULE_ADDR 4
#define MAX_SCHEDULE_ITEMS 20

// ==================== DATA STRUCTURES ====================
struct BellEvent {
  uint8_t hour;
  uint8_t minute;
  bool active;
  uint8_t duration;
};

// ==================== GLOBAL VARIABLES ====================
WebServer server(80);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
RTC_DS3231 rtc;

BellEvent schedule[MAX_SCHEDULE_ITEMS];
int eventCount = 0;
bool isRinging = false;
int currentBellDuration = 3;
int lastRingMinute = -1;

// Cache variables to prevent unnecessary updates
int lastSecond = -1;
int lastMinute = -1;
int lastEventCount = -1;
String lastNextBell = "";
bool lastWifiStatus = false;
int lastDay = -1;
String lastTimeStr = "";

// ==================== HTML WEB INTERFACE ====================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>School Bell System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
        button { background: #4CAF50; color: white; padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        button.red { background: #f44336; }
        .schedule-item { background: #e0e0e0; margin: 5px; padding: 10px; border-radius: 5px; display: flex; justify-content: space-between; }
        .status { padding: 20px; margin: 10px 0; border-radius: 5px; text-align: center; }
        .ringing { background: #ff9800; color: white; }
        .idle { background: #4CAF50; color: white; }
        input { padding: 8px; margin: 5px; width: 80px; }
    </style>
    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('time').innerText = d.time;
                    document.getElementById('next').innerText = d.next || 'None';
                    document.getElementById('count').innerText = d.count;
                    let s = document.getElementById('status');
                    if(d.ringing) {
                        s.className = 'status ringing';
                        s.innerHTML = '<h2>🔔 BELL RINGING!</h2>';
                    } else {
                        s.className = 'status idle';
                        s.innerHTML = '<h2>⏰ System Active</h2>';
                    }
                });
        }
        
        function ringBell() {
            fetch('/api/ring', {method: 'POST'}).then(() => updateStatus());
        }
        
        function addSchedule() {
            let hour = document.getElementById('hour').value;
            let minute = document.getElementById('minute').value;
            let duration = document.getElementById('duration').value;
            fetch('/api/add', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({hour: parseInt(hour), minute: parseInt(minute), duration: parseInt(duration)})
            }).then(() => location.reload());
        }
        
        function deleteSchedule(idx) {
            fetch('/api/delete', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({index: idx})
            }).then(() => location.reload());
        }
        
        setInterval(updateStatus, 1000);
        window.onload = updateStatus;
    </script>
</head>
<body>
    <div class="container">
        <h1>🏫 School Bell System</h1>
        <div id="status" class="status idle"><h2>⏰ System Active</h2></div>
        <h2>Time: <span id="time">--:--:--</span></h2>
        <h2>Next Bell: <span id="next">--:--</span></h2>
        <h2>Events: <span id="count">0</span></h2>
        <button onclick="ringBell()">🔔 MANUAL BELL</button>
        
        <h3>Add Schedule</h3>
        <input type="number" id="hour" placeholder="Hour" min="0" max="23">
        <input type="number" id="minute" placeholder="Minute" min="0" max="59">
        <input type="number" id="duration" placeholder="Sec" value="3" min="1" max="10">
        <button onclick="addSchedule()">Add</button>
        
        <h3>Schedule List</h3>
        <div id="scheduleList"></div>
        
        <script>
            fetch('/api/schedule').then(r => r.json()).then(s => {
                let html = '';
                s.forEach((e, i) => {
                    html += `<div class="schedule-item">
                        <span>${String(e.hour).padStart(2,'0')}:${String(e.minute).padStart(2,'0')} (${e.duration}s)</span>
                        <button class="red" onclick="deleteSchedule(${i})">Delete</button>
                    </div>`;
                });
                document.getElementById('scheduleList').innerHTML = html || '<p>No schedules</p>';
            });
        </script>
    </div>
</body>
</html>
)rawliteral";

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
  tft.initR(INITR_BLACKTAB);  // For ST7735S
  tft.setRotation(1);  // Landscape mode (160x128)
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
  
  // Draw static elements (only once)
  drawStaticElements();
}

void drawStaticElements() {
  // Title (static)
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 5);
  tft.print("SCHOOL BELL SYSTEM");
  
  // Labels
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 65);
  tft.print("Next:");
  
  tft.setCursor(5, 85);
  tft.print("Events:");
  
  tft.setCursor(5, 105);
  tft.print("Date:");
  
  tft.setCursor(5, 125);
  tft.print("WiFi:");
  
  // Draw separator line
  tft.drawLine(0, 145, 160, 145, ST77XX_BLUE);
}

void updateDisplay() {
  DateTime now = rtc.now();
  
  // === UPDATE TIME (changes every second) ===
  char timeBuf[20];
  snprintf(timeBuf, 20, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  String currentTimeStr = String(timeBuf);
  
  if (currentTimeStr != lastTimeStr) {
    // Clear only the time area
    tft.fillRect(5, 30, 150, 30, ST77XX_BLACK);
    
    // Draw new time in large font
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(5, 30);
    tft.print(currentTimeStr);
    
    lastTimeStr = currentTimeStr;
    lastSecond = now.second();
  }
  
  // === UPDATE NEXT BELL (only when minute changes) ===
  if (now.minute() != lastMinute) {
    String nextBell = getNextBellTime();
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
  
  // === UPDATE EVENT COUNT ===
  if (eventCount != lastEventCount) {
    tft.fillRect(60, 85, 80, 15, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(60, 85);
    tft.print(eventCount);
    lastEventCount = eventCount;
  }
  
  // === UPDATE DATE ===
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
  
  // === UPDATE WIFI STATUS ===
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
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
  
  // === UPDATE RINGING STATUS ===
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
  
  // Reset text color to default
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
}

// ==================== RTC FUNCTIONS ====================
bool initRTC() {
  Serial.println("Initializing RTC...");
  Wire.begin(RTC_SDA, RTC_SCL);
  
  if (!rtc.begin()) {
    Serial.println("RTC NOT FOUND!");
    return false;
  }
  
  Serial.println("RTC found!");
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power - setting time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  DateTime now = rtc.now();
  Serial.printf("RTC Time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  return true;
}

// ==================== WIFI FUNCTIONS ====================
void initWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed - continuing without WiFi");
  }
}

// ==================== SCHEDULE FUNCTIONS ====================
void loadSchedule() {
  int magic;
  EEPROM.get(0, magic);
  
  if (magic == MAGIC_NUMBER) {
    EEPROM.get(SCHEDULE_ADDR, eventCount);
    EEPROM.get(SCHEDULE_ADDR + sizeof(eventCount), schedule);
    Serial.printf("Loaded %d schedules from EEPROM\n", eventCount);
  } else {
    Serial.println("No schedule found, adding default");
    addDefaultSchedule();
  }
}

void saveSchedule() {
  int magic = MAGIC_NUMBER;
  EEPROM.put(0, magic);
  EEPROM.put(SCHEDULE_ADDR, eventCount);
  EEPROM.put(SCHEDULE_ADDR + sizeof(eventCount), schedule);
  EEPROM.commit();
  Serial.printf("Saved %d schedules to EEPROM\n", eventCount);
}

void addDefaultSchedule() {
  eventCount = 0;
  // Typical school schedule: hour, minute, duration(seconds)
  int times[][3] = {
    {8, 0, 3},    // School starts
    {9, 30, 2},   // Break
    {10, 30, 2},  // Recess
    {12, 0, 3},   // Lunch
    {14, 0, 2},   // Afternoon
    {15, 30, 3}   // Home time
  };
  
  for(int i = 0; i < 6 && eventCount < MAX_SCHEDULE_ITEMS; i++) {
    schedule[eventCount].hour = times[i][0];
    schedule[eventCount].minute = times[i][1];
    schedule[eventCount].duration = times[i][2];
    schedule[eventCount].active = true;
    eventCount++;
  }
  saveSchedule();
  Serial.println("Default schedule added");
}

// ==================== BELL FUNCTIONS ====================
void ringBell(int duration) {
  if (isRinging) return;
  
  isRinging = true;
  Serial.printf("BELL RINGING for %d seconds\n", duration);
  
  // Turn on LEDs
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_WHITE, HIGH);
  
  // Ring buzzer
  unsigned long startTime = millis();
  while (millis() - startTime < (duration * 1000)) {
    tone(BUZZER_PIN, 2000);
    delay(80);
    noTone(BUZZER_PIN);
    delay(20);
    server.handleClient();
  }
  
  noTone(BUZZER_PIN);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_WHITE, LOW);
  
  isRinging = false;
  Serial.println("Bell stopped");
  
  // Update display to remove ringing text
  updateDisplay();
}

void checkSchedule() {
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();
  int currentSecond = now.second();
  
  // Check at the start of each minute (second 0)
  if (currentSecond == 0) {
    for (int i = 0; i < eventCount; i++) {
      if (schedule[i].active && 
          schedule[i].hour == currentHour && 
          schedule[i].minute == currentMinute &&
          currentMinute != lastRingMinute) {
        
        ringBell(schedule[i].duration);
        lastRingMinute = currentMinute;
        Serial.printf("Scheduled bell at %02d:%02d\n", currentHour, currentMinute);
        break;
      }
    }
  }
}

// ==================== WEB SERVER ====================
void setupWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });
  
  server.on("/api/status", HTTP_GET, []() {
    DateTime now = rtc.now();
    char timeStr[20];
    snprintf(timeStr, 20, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    
    String json = "{";
    json += "\"time\":\"" + String(timeStr) + "\",";
    json += "\"next\":\"" + getNextBellTime() + "\",";
    json += "\"ringing\":" + String(isRinging ? "true" : "false") + ",";
    json += "\"count\":" + String(eventCount);
    json += "}";
    server.send(200, "application/json", json);
  });
  
  server.on("/api/schedule", HTTP_GET, []() {
    String json = "[";
    for(int i = 0; i < eventCount; i++) {
      if(i > 0) json += ",";
      json += "{\"hour\":" + String(schedule[i].hour) + 
              ",\"minute\":" + String(schedule[i].minute) + 
              ",\"duration\":" + String(schedule[i].duration) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  
  server.on("/api/add", HTTP_POST, []() {
    if(server.hasArg("plain")) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, server.arg("plain"));
      if(eventCount < MAX_SCHEDULE_ITEMS) {
        schedule[eventCount].hour = doc["hour"];
        schedule[eventCount].minute = doc["minute"];
        schedule[eventCount].duration = doc["duration"];
        schedule[eventCount].active = true;
        eventCount++;
        saveSchedule();
        
        // Visual feedback - green LED flash
        digitalWrite(LED_GREEN, HIGH);
        delay(200);
        digitalWrite(LED_GREEN, LOW);
        
        Serial.printf("Schedule added: %02d:%02d (%ds)\n", 
                      doc["hour"].as<int>(), doc["minute"].as<int>(), doc["duration"].as<int>());
      }
    }
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  server.on("/api/delete", HTTP_POST, []() {
    if(server.hasArg("plain")) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, server.arg("plain"));
      int index = doc["index"];
      if(index >= 0 && index < eventCount) {
        for(int i = index; i < eventCount - 1; i++) {
          schedule[i] = schedule[i + 1];
        }
        eventCount--;
        saveSchedule();
        Serial.printf("Schedule deleted at index %d\n", index);
      }
    }
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  server.on("/api/ring", HTTP_POST, []() {
    ringBell(currentBellDuration);
    server.send(200, "application/json", "{\"status\":\"ringing\"}");
  });
  
  server.begin();
  Serial.println("Web server started on port 80");
}

String getNextBellTime() {
  DateTime now = rtc.now();
  int currentTotal = now.hour() * 60 + now.minute();
  int nextTotal = 24 * 60;
  int nextHour = -1, nextMinute = -1;
  
  for(int i = 0; i < eventCount; i++) {
    if(schedule[i].active) {
      int eventTotal = schedule[i].hour * 60 + schedule[i].minute;
      if(eventTotal > currentTotal && eventTotal < nextTotal) {
        nextTotal = eventTotal;
        nextHour = schedule[i].hour;
        nextMinute = schedule[i].minute;
      }
    }
  }
  
  // If no more bells today, show first bell tomorrow
  if(nextHour == -1 && eventCount > 0) {
    for(int i = 0; i < eventCount; i++) {
      if(schedule[i].active) {
        if(nextHour == -1 || 
           (schedule[i].hour * 60 + schedule[i].minute) < (nextHour * 60 + nextMinute)) {
          nextHour = schedule[i].hour;
          nextMinute = schedule[i].minute;
        }
      }
    }
    if(nextHour >= 0) {
      char buf[20];
      snprintf(buf, 20, "%02d:%02d*", nextHour, nextMinute);
      return String(buf);
    }
  }
  
  if(nextHour >= 0) {
    char buf[10];
    snprintf(buf, 10, "%02d:%02d", nextHour, nextMinute);
    return String(buf);
  }
  
  return "None";
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

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n==========================================");
  Serial.println("School Bell System - ST7735S Optimized");
  Serial.println("==========================================\n");
  
  // Initialize LEDs
  initLEDs();
  digitalWrite(LED_BLUE, HIGH);  // Blue LED = booting
  delay(500);
  
  // Initialize Display
  initDisplay();
  
  // Initialize RTC
  if (!initRTC()) {
    Serial.println("ERROR: RTC not found!");
    digitalWrite(LED_RED, HIGH);  // Red LED = error
    while(1) {
      delay(1000);
      Serial.println("Check RTC wiring: + to 3.3V, - to GND, D to 21, C to 22");
    }
  }
  
  // Initialize EEPROM and load schedule
  EEPROM.begin(EEPROM_SIZE);
  loadSchedule();
  
  // Initialize WiFi (optional)
  initWiFi();
  
  // Setup Web Server
  setupWebServer();
  
  // Ready!
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, HIGH);  // Green LED = system ready
  delay(500);
  digitalWrite(LED_GREEN, LOW);
  
  Serial.println("\n✓ System Ready!");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("  Web Interface: http://");
    Serial.println(WiFi.localIP());
  }
  Serial.println("==========================================\n");
  
  // Initial display update
  updateDisplay();
}

// ==================== LOOP ====================
void loop() {
  static unsigned long lastCheck = 0;
  static unsigned long lastDisplay = 0;
  
  // Handle web requests
  server.handleClient();
  
  // Check schedule every second
  if (millis() - lastCheck >= 1000) {
    checkSchedule();
    lastCheck = millis();
  }
  
  // Update display only when needed (max 10 times per second)
  if (millis() - lastDisplay >= 100) {
    updateDisplay();
    lastDisplay = millis();
  }
  
  // Heartbeat LED
  heartbeatLED();
  
  delay(10);
}   
