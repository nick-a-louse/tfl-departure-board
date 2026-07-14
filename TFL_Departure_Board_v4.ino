#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include "time.h"
#include <vector>
#include <algorithm>
#include "secrets.h"
#include <Preferences.h>

// Custom font (FreeSans20pt7b.h must be in your sketch folder)
#include "FreeSans20pt7b.h"

// --- Configuration ---
const char* TZ_INFO = "GMT0BST,M3.5.0/1,M10.5.0/2";

// --- TfL Stop IDs ---
const String trainStop = "910GEXAMPLE";
const String busStopPR  = "490000000A";  
const String busStopPA = "490000000B";  
const String busStopAP  = "490000000C";  
const String busStopAC  = "490000000D";  

// --- Display & Hardware Setup ---
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(GxEPD2_750_GDEY075T7(13, 22, 21, 14));

const int PIN_BAT_ADC = A2;      
const int timeColumnX = 400;    
const int startX = 20;          

struct Departure {
  String destination;
  String line;
  time_t arrivalRaw;            
  int hour;
  int minute;
};

// --- ICON DRAWING FUNCTIONS ---

void drawTrainIcon(int x, int y) {
  display.drawLine(x - 5, y + 6,  x + 3, y + 6,  GxEPD_BLACK);
  display.drawLine(x - 8, y + 9,  x + 3, y + 9,  GxEPD_BLACK);
  display.drawLine(x - 3, y + 12, x + 3, y + 12, GxEPD_BLACK);
  int currentX = x + 5;
  display.drawLine(currentX + 4, y, currentX + 27, y, GxEPD_BLACK);          
  display.drawLine(currentX + 27, y, currentX + 27, y + 13, GxEPD_BLACK);    
  display.drawLine(currentX + 27, y + 13, currentX, y + 13, GxEPD_BLACK);    
  display.drawLine(currentX, y + 13, currentX + 4, y, GxEPD_BLACK);          
  display.drawLine(currentX + 14, y + 3, currentX + 14, y + 7, GxEPD_BLACK);
  display.drawLine(currentX + 14, y + 3, currentX + 3, y + 3, GxEPD_BLACK);  
  display.drawLine(currentX + 14, y + 7, currentX + 1, y + 7, GxEPD_BLACK);  
  display.drawRect(currentX + 17, y + 3, 7, 5, GxEPD_BLACK);
  display.drawLine(currentX + 11, y, currentX + 15, y - 4, GxEPD_BLACK);
  display.drawLine(currentX + 15, y - 4, currentX + 19, y, GxEPD_BLACK);
  display.drawLine(currentX + 11, y - 4, currentX + 19, y - 4, GxEPD_BLACK);
  display.fillCircle(currentX + 8, y + 15, 2, GxEPD_BLACK);    
  display.fillCircle(currentX + 23, y + 15, 2, GxEPD_BLACK);
  display.drawLine(currentX + 27, y + 10, currentX + 32, y + 10, GxEPD_BLACK);
  currentX += 32;
  for (int i = 0; i < 5; i++) {
    display.drawRect(currentX, y, 24, 14, GxEPD_BLACK);
    display.drawRect(currentX + 4, y + 3, 7, 5, GxEPD_BLACK);
    display.drawRect(currentX + 13, y + 3, 7, 5, GxEPD_BLACK);
    display.fillCircle(currentX + 6, y + 15, 2, GxEPD_BLACK);
    display.fillCircle(currentX + 18, y + 15, 2, GxEPD_BLACK);
    display.drawLine(currentX + 24, y + 10, currentX + 27, y + 10, GxEPD_BLACK);
    currentX += 27;
  }
  display.drawLine(currentX, y, currentX + 23, y, GxEPD_BLACK);              
  display.drawLine(currentX + 23, y, currentX + 27, y + 13, GxEPD_BLACK);    
  display.drawLine(currentX + 27, y + 13, currentX, y + 13, GxEPD_BLACK);    
  display.drawLine(currentX, y + 13, currentX, y, GxEPD_BLACK);              
  display.drawRect(currentX + 4, y + 3, 7, 5, GxEPD_BLACK);
  display.drawLine(currentX + 14, y + 3, currentX + 14, y + 7, GxEPD_BLACK);
  display.drawLine(currentX + 14, y + 3, currentX + 24, y + 3, GxEPD_BLACK);
  display.drawLine(currentX + 14, y + 7, currentX + 26, y + 7, GxEPD_BLACK);
  display.drawLine(currentX + 8, y, currentX + 12, y - 4, GxEPD_BLACK);
  display.drawLine(currentX + 12, y - 4, currentX + 16, y, GxEPD_BLACK);
  display.drawLine(currentX + 8, y - 4, currentX + 16, y - 4, GxEPD_BLACK);
  display.fillCircle(currentX + 4, y + 15, 2, GxEPD_BLACK);    
  display.fillCircle(currentX + 19, y + 15, 2, GxEPD_BLACK);  
}

void drawBusIcon(int x, int y) {
  display.drawLine(x - 10, y + 5, x - 2, y + 5, GxEPD_BLACK);
  display.drawLine(x - 14, y + 10, x - 2, y + 10, GxEPD_BLACK);
  display.drawLine(x - 8, y + 15, x - 2, y + 15, GxEPD_BLACK);
  display.fillCircle(x + 8, y + 16, 3, GxEPD_BLACK);
  display.fillCircle(x + 26, y + 16, 3, GxEPD_BLACK);
  display.drawRoundRect(x, y - 4, 34, 20, 3, GxEPD_BLACK);
  display.drawLine(x + 1, y + 6, x + 33, y + 6, GxEPD_BLACK);
  display.drawRect(x + 2, y - 2, 9, 5, GxEPD_BLACK);  
  display.drawRect(x + 13, y - 2, 7, 5, GxEPD_BLACK);  
  display.drawRect(x + 22, y - 2, 12, 5, GxEPD_BLACK);  
  display.drawRect(x + 2, y + 8, 9, 5, GxEPD_BLACK);  
  display.drawRect(x + 13, y + 8, 7, 8, GxEPD_BLACK);
  display.drawLine(x + 16, y + 8, x + 16, y + 16, GxEPD_BLACK);
  display.drawLine(x + 22, y + 8, x + 32, y + 8, GxEPD_BLACK);  
  display.drawLine(x + 22, y + 12, x + 32, y + 12, GxEPD_BLACK);
  display.drawLine(x + 22, y + 12, x + 22, y + 8, GxEPD_BLACK);  
}

void drawStickQueue(int startX, int groundY) {
  int x = startX;
  display.fillCircle(x, groundY - 28, 3, GxEPD_BLACK);
  display.drawLine(x, groundY - 25, x, groundY - 12, GxEPD_BLACK);
  display.drawLine(x, groundY - 22, x - 5, groundY - 16, GxEPD_BLACK);
  display.drawLine(x, groundY - 22, x + 5, groundY - 16, GxEPD_BLACK);
  display.drawLine(x, groundY - 12, x - 4, groundY, GxEPD_BLACK);
  display.drawLine(x, groundY - 12, x + 4, groundY, GxEPD_BLACK);
  x += 18;
  display.fillCircle(x, groundY - 26, 3, GxEPD_BLACK);
  display.drawLine(x, groundY - 23, x, groundY - 10, GxEPD_BLACK);
  display.drawLine(x - 5, groundY - 10, x + 5, groundY - 10, GxEPD_BLACK);
  display.drawLine(x, groundY - 23, x - 5, groundY - 10, GxEPD_BLACK);
  display.drawLine(x, groundY - 23, x + 5, groundY - 10, GxEPD_BLACK);
  display.drawLine(x, groundY - 21, x - 6, groundY - 17, GxEPD_BLACK);
  display.drawLine(x, groundY - 21, x + 6, groundY - 17, GxEPD_BLACK);
  display.drawLine(x - 2, groundY - 10, x - 2, groundY, GxEPD_BLACK);
  display.drawLine(x + 2, groundY - 10, x + 2, groundY, GxEPD_BLACK);
  x += 16;
  display.fillCircle(x, groundY - 18, 2, GxEPD_BLACK);
  display.drawLine(x, groundY - 16, x, groundY - 7, GxEPD_BLACK);
  display.drawLine(x, groundY - 14, x - 3, groundY - 10, GxEPD_BLACK);
  display.drawLine(x, groundY - 14, x + 3, groundY - 10, GxEPD_BLACK);
  display.drawLine(x, groundY - 7, x - 3, groundY, GxEPD_BLACK);
  display.drawLine(x, groundY - 7, x + 3, groundY, GxEPD_BLACK);
  x += 12;
  display.fillCircle(x, groundY - 15, 2, GxEPD_BLACK);
  display.drawLine(x, groundY - 13, x, groundY - 6, GxEPD_BLACK);
  display.drawLine(x - 4, groundY - 6, x + 4, groundY - 6, GxEPD_BLACK);
  display.drawLine(x, groundY - 13, x - 4, groundY - 6, GxEPD_BLACK);
  display.drawLine(x, groundY - 13, x + 4, groundY - 6, GxEPD_BLACK);
  display.drawLine(x, groundY - 12, x - 3, groundY - 9, GxEPD_BLACK);
  display.drawLine(x, groundY - 12, x + 3, groundY - 9, GxEPD_BLACK);
  display.drawLine(x - 2, groundY - 6, x - 2, groundY, GxEPD_BLACK);
  display.drawLine(x + 2, groundY - 6, x + 2, groundY, GxEPD_BLACK);
  x += 15;
  display.fillCircle(x + 8, groundY - 8, 2, GxEPD_BLACK);
  display.drawRect(x, groundY - 7, 8, 4, GxEPD_BLACK);
  display.drawLine(x, groundY - 3, x, groundY, GxEPD_BLACK);
  display.drawLine(x + 2, groundY - 3, x + 2, groundY, GxEPD_BLACK);
  display.drawLine(x + 6, groundY - 3, x + 6, groundY, GxEPD_BLACK);
  display.drawLine(x + 8, groundY - 3, x + 8, groundY, GxEPD_BLACK);
  display.drawLine(x, groundY - 7, x - 1, groundY - 10, GxEPD_BLACK);
}

// --- CORE UTILS ---

bool compareDepartures(const Departure &a, const Departure &b) { return a.arrivalRaw < b.arrivalRaw; }

void drawDottedLine(int x, int y, int len) {
  for (int i = 0; i < len; i += 4) {
    display.drawPixel(x + i, y, GxEPD_BLACK);
    display.drawPixel(x + i + 1, y, GxEPD_BLACK);
  }
}

void drawTfLRoundel(int x, int y, int radius) {
  display.setFont(NULL);
  for (int i = 0; i < 5; i++) display.drawCircle(x, y, radius - i, GxEPD_BLACK);
  display.fillRect(x - (radius * 1.25), y - (radius * 0.37), radius * 2.5, radius * 0.75, GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);
  display.setTextSize(1);
  display.setCursor(x - 8, y - 4); display.print("TfL");
  display.setTextColor(GxEPD_BLACK);
}

// TfL returns UTC timestamps. mktime() assumes local time, so we
// calculate and subtract the UTC offset to compensate.

time_t parseISO8601(const char* isoTime) {
  if (!isoTime || strlen(isoTime) < 19) return 0;
  struct tm t = {0};
  int year, month, day, hour, minute, second;
  if (sscanf(isoTime, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
    t.tm_year = year - 1900; t.tm_mon = month - 1; t.tm_mday = day;
    t.tm_hour = hour; t.tm_min = minute; t.tm_sec = second;
    t.tm_isdst = 0;
    // mktime() assumes local time, but TfL sends UTC.
    // Convert to UTC by subtracting the local timezone offset.
    time_t local = mktime(&t);
    time_t utcNow = time(nullptr);
    struct tm *gmNow = gmtime(&utcNow);
    struct tm *localNow = localtime(&utcNow);
    int offsetSeconds = (int)difftime(mktime(localNow), mktime(gmNow));
    return local - offsetSeconds;
  }
  return 0;
}

bool lineMatches(String filterLines, String lineName) {
  if (filterLines.length() == 0) return true;
  int start = 0;
  int comma = filterLines.indexOf(',', start);
  while (comma != -1) {
    if (filterLines.substring(start, comma) == lineName) return true;
    start = comma + 1;
    comma = filterLines.indexOf(',', start);
  }
  return filterLines.substring(start) == lineName;
}

String fetchPayload(String stopId) {
  for (int attempt = 0; attempt < 3; attempt++) {
    HTTPClient http;
    http.setTimeout(30000);
    String url = "https://api.tfl.gov.uk/StopPoint/" + stopId + "/Arrivals?app_key=" + String(tfl_app_key);
    http.begin(url);
    int httpCode = http.GET();
    Serial.print("HTTP code for ");
    Serial.print(stopId);
    Serial.print(": ");
    Serial.println(httpCode);
    if (httpCode == 200) {
      String payload = http.getString();
      http.end();
      if (payload.length() > 0) {
        Serial.print("Payload length: ");
        Serial.println(payload.length());
        return payload;
      }
      Serial.println("Empty payload — retrying...");
    } else {
      http.end();
    }
    delay(500);
  }
  Serial.println("All retries failed.");
  return "";
}

void renderDepartures(String payload, int yOffset, time_t now, bool isTrain, int showLimit, String filterLines) {
  // Serial.printf("now = %ld\n", (long)now);
  display.setFont(NULL);
  display.setTextSize(2);
  display.setTextColor(GxEPD_BLACK);

  Serial.print("Payload length for render: ");
  Serial.println(payload.length());
  if (payload.length() == 0 || payload.startsWith("ERROR:")) {
    display.setCursor(startX, yOffset);
    display.print(payload.startsWith("ERROR:") ? payload.substring(6) : "No Data");
    return;
  }

  StaticJsonDocument<128> filter;
  filter[0]["lineName"] = true;
  filter[0]["destinationName"] = true;
  filter[0]["expectedArrival"] = true;

  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (error) {
    // Serial.print("JSON parse failed: ");
    // Serial.println(error.c_str());
    display.setCursor(startX, yOffset);
    display.print("Data error");
    return;
  }

  JsonArray array = doc.as<JsonArray>();
  std::vector<Departure> departures;

  for (JsonObject obj : array) {
    const char* lineCStr = obj["lineName"] | (isTrain ? "ELIZ" : "BUS");
    String lineName = String(lineCStr);
    // Serial.print("Line: [");
    // Serial.print(lineName);
    // Serial.println("]");
    if (!isTrain && filterLines.length() > 0) {
      if (!lineMatches(filterLines, lineName)) continue;
    }
    const char* arrivalCStr = obj["expectedArrival"] | "";
    if (strlen(arrivalCStr) > 0) {
      time_t arrivalTime = parseISO8601(arrivalCStr);
      // Serial.printf("arrival = %ld, diff = %.0f\n", (long)arrivalTime, difftime(arrivalTime, now));
      if (arrivalTime == 0) continue;
      if (difftime(arrivalTime, now) < 120) continue;
      struct tm *lt = localtime(&arrivalTime);
      if (lt == nullptr) continue;
      String dest = String((const char*)(obj["destinationName"] | "Unknown"));
      dest.replace(" Rail Station", "");
      dest.replace(" (TfL)", "");
      dest.trim();
      departures.push_back({dest, lineName, arrivalTime, lt->tm_hour, lt->tm_min});
    }
  }

  std::sort(departures.begin(), departures.end(), compareDepartures);

  if (departures.empty()) {
    display.setCursor(startX, yOffset);
    display.print("No departures in the next 30 minutes");
  } else {
    int displayed = 0;
    for (const auto& dep : departures) {
      if (displayed >= showLimit) break;
      int currentY = yOffset + (displayed * 26);
      display.setCursor(startX, currentY);
      if (isTrain) display.printf("%-29s", dep.destination.substring(0, 28).c_str());
      else display.printf("%-4s %-25s", dep.line.c_str(), dep.destination.substring(0, 24).c_str());
      display.setCursor(timeColumnX, currentY);
      display.printf("%02d:%02d", dep.hour, dep.minute);
      displayed++;
    }
  }
}

void drawBoxedSection(String title, int y, bool isTrain, uint16_t t_w) {
  display.drawRoundRect(10, y, 460, (isTrain ? 185 : 100), 12, GxEPD_BLACK);
  display.setCursor(startX, y + 15);
  display.print(title);
  int padding = isTrain ? 15 : 20;
  int iconX = startX + t_w + padding;
  if (isTrain) drawTrainIcon(iconX, y + 15);
  else drawBusIcon(iconX, y + 15);
  drawDottedLine(startX, y + 35, 440);
}

int getBattery() {
  int raw = analogRead(PIN_BAT_ADC);
 
  // Calibrated multiplier: 2.189 (was 2.1)
  float v = (raw / 4095.0) * 3.3 * 2.189;

  // Serial.print("--- BATTERY DEBUG --- Raw: ");
  // Serial.print(raw);
  // Serial.print(" | Calibrated Volts: ");
  // Serial.println(v);

  if (v < 3.3) {
    Serial.println("WARNING: Voltage below 3.3V — battery critical or calibration drift.");
  }

  // Map 3.3V to 0% and 4.17V to 100%
  int pct = (int)((v - 3.3) / (4.17 - 3.3) * 100);
 
  // Keep the result between 0 and 100
  return std::max(0, std::min(100, pct));
}

void updateBoard() {
  time_t now; struct tm tinfo;
  if (!getLocalTime(&tinfo)) return;
  time(&now);
  int batPct = getBattery();

  // --- Fetch all data before touching the display ---
  Serial.print("Free heap before fetch: ");
  Serial.println(ESP.getFreeHeap());

  String payloadTrain = fetchPayload(trainStop);
  Serial.print("Free heap after train: ");
  Serial.println(ESP.getFreeHeap());

  String payloadPR = fetchPayload(busStopPR);
  Serial.print("Free heap after PR: ");
  Serial.println(ESP.getFreeHeap());

  String payloadPA = fetchPayload(busStopPA);
  Serial.print("Free heap after PA: ");
  Serial.println(ESP.getFreeHeap());

  String payloadAP = fetchPayload(busStopAP);
  Serial.print("Free heap after AP: ");
  Serial.println(ESP.getFreeHeap());

  String payloadAC = fetchPayload(busStopAC);
  Serial.print("Free heap after AC: ");
  Serial.println(ESP.getFreeHeap());

  // --- Now draw everything ---
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSans20pt7b);
    display.setTextSize(1);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(startX, 52); display.print("TfL DEPARTURES");
    drawTfLRoundel(435, 40, 25);
    display.drawLine(0, 80, 480, 80, GxEPD_BLACK);
    display.setFont(NULL);
    display.setTextSize(2);

    int16_t t_x1, t_y1; uint16_t t_w, t_h;

    display.getTextBounds("Example Station", startX, 90 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Station", 90, true, t_w);
    renderDepartures(payloadTrain, 90 + 45, now, true, 5, "");

    display.getTextBounds("Example Road (PR)", startX, 285 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Road (PR)", 285, false, t_w);
    renderDepartures(payloadPR, 285 + 45, now, false, 2, "");

    display.getTextBounds("Example Station (PA)", startX, 395 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Station (PA)", 395, false, t_w);
    renderDepartures(payloadPA, 395 + 45, now, false, 2, "496");

    display.getTextBounds("Example Roundabout (AP)", startX, 505 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Roundabout (AP)", 505, false, t_w);
    renderDepartures(payloadAP, 505 + 45, now, false, 2, "370");

    display.getTextBounds("Example Roundabout (AC)", startX, 615 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Roundabout (AC)", 615, false, t_w);
    renderDepartures(payloadAC, 615 + 45, now, false, 2, "165,370");

    int footerY = 750;
    int footerH = 35;
    int16_t x1, y1; uint16_t w, h;
    char timeStr[20];
    sprintf(timeStr, "Updated: %02d:%02d", tinfo.tm_hour, tinfo.tm_min);
    display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    int timeBoxW = w + 20;
    display.drawRoundRect(startX, footerY, timeBoxW, footerH, 8, GxEPD_BLACK);
    display.setCursor(startX + 10, footerY + 10);
    display.print(timeStr);
    drawStickQueue(startX + 20, footerY);
    String batStr = "Battery: " + String(batPct) + "%";
    display.getTextBounds(batStr, 0, 0, &x1, &y1, &w, &h);
    int batBoxW = w + 20;
    int batteryBoxX = 480 - batBoxW - 20;
    display.drawRoundRect(batteryBoxX, footerY, batBoxW, footerH, 8, GxEPD_BLACK);
    display.setCursor(batteryBoxX + 10, footerY + 10);
    display.print(batStr);
  } while (display.nextPage());
}

// --- SYSTEM MANAGEMENT ---

void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  struct tm timeinfo;

  // 1. Initialize Display
  display.init(115200);

  // 2. Optimized Wi-Fi Connection — uses saved channel/BSSID to skip network scan
  Preferences prefs;
  prefs.begin("wifi", true);
  int savedChannel = prefs.getInt("channel", 0);
  uint8_t savedBssid[6];
  bool hasSaved = prefs.getBytes("bssid", savedBssid, 6) == 6;
  prefs.end();

  if (hasSaved && savedChannel > 0) {
    WiFi.begin(ssid, password, savedChannel, savedBssid);
  } else {
    WiFi.begin(ssid, password);
  }

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    attempt++;
  }

  // Save channel and BSSID after successful connection
  if (WiFi.status() == WL_CONNECTED) {
    prefs.begin("wifi", false);
    prefs.putInt("channel", WiFi.channel());
    prefs.putBytes("bssid", WiFi.BSSID(), 6);
    prefs.end();
  }

  // 3. Update if connected
  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");

    // Wait for NTP sync — retry up to 10 times
    bool timeSynced = false;
    for (int i = 0; i < 10; i++) {
      if (getLocalTime(&timeinfo)) {
        timeSynced = true;
        break;
      }
      Serial.println("Waiting for NTP sync...");
      delay(500);
    }

    if (timeSynced) {
      updateBoard();
    } else {
      Serial.println("Time sync failed — skipping update.");
      display.setRotation(1);
      // Optional: show a message on the display
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(NULL);
        display.setTextSize(2);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(20, 40);
        display.print("No time sync - retrying in 5m");
      } while (display.nextPage());
    }
  } else {
    Serial.println("Wi-Fi failed — showing error on display.");
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.setFont(NULL);
      display.setTextSize(2);
      display.setTextColor(GxEPD_BLACK);
      display.setCursor(20, 40);
      display.print("No Wi-Fi connection.");
      display.setCursor(20, 70);
      display.print("Retrying in 5 minutes.");
    } while (display.nextPage());
  }
 
  // 4. Force Wi-Fi Radio Off to save power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop(); // Also stop Bluetooth just in case

  // 5. Deep Sleep — longer at night to save battery
  int sleepMinutes;
  if (getLocalTime(&timeinfo)) {
    int hour = timeinfo.tm_hour;
    if (hour >= 23 || hour < 6) {
      sleepMinutes = 30;  // 30 min overnight (11pm - 6am)
    } else if (hour >= 20) {
      sleepMinutes = 15;  // 15 min in the evening (8pm - 11pm)
    } else {
      sleepMinutes = 5;   // 5 min during the day
    }
  } else {
    sleepMinutes = 5;     // Default if time unavailable
  }

  Serial.print("Entering Deep Sleep for ");
  Serial.print(sleepMinutes);
  Serial.println(" minutes...");
  esp_sleep_enable_timer_wakeup((uint64_t)sleepMinutes * 60 * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Setup runs once, then sleep restarts the ESP32. Loop remains empty.
}