// All layout, positions, fonts and drawing calls in this file are ported
// unchanged from the original sketch - this is the visual output and must
// stay pixel-identical. Only the data plumbing feeding into it (fetch/parse/
// timing/power) changed elsewhere.

#include "renderer.h"

#include <GxEPD2_BW.h>

#include "Fonts/FreeSans20pt7b.h"
#include "Fonts/FreeSans_26pt8b.h"
// Icons ported from esp32-weather-epd (Weather_Picture_Frame):
//  - wifi_x: Phosphor Icons (https://github.com/phosphor-icons/homepage), MIT license
//  - battery_alert: Google Fonts Material Symbols, Apache License 2.0
#include "Icons/wifi_x_196x196.h"
#include "Icons/battery_alert_0deg_196x196.h"

namespace {

const int timeColumnX = 400;
const int startX = 20;

GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(
    GxEPD2_750_GDEY075T7(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));

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

// Centers a single line of text horizontally around cx, at baseline y.
void drawCenteredText(const String& text, int cx, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(cx - (int)(w / 2) - x1, y);
  display.print(text);
}

// Greedy word-wrap, each line centered around cx. Weather_Picture_Frame's
// reference screen is 800px wide so single-line error text fits there;
// this panel is only 480px wide in this rotation, so the same text at the
// same font size needs wrapping to avoid running off the edge.
void drawWrappedCenteredText(const String& text, int cx, int startY, int maxWidth, int lineHeight) {
  int16_t x1, y1;
  uint16_t w, h;
  String line;
  int y = startY;
  int start = 0;
  while (start < (int)text.length()) {
    int spaceIdx = text.indexOf(' ', start);
    String word = (spaceIdx == -1) ? text.substring(start) : text.substring(start, spaceIdx);
    String candidate = line.length() == 0 ? word : line + " " + word;
    display.getTextBounds(candidate, 0, 0, &x1, &y1, &w, &h);
    if ((int)w > maxWidth && line.length() > 0) {
      drawCenteredText(line, cx, y);
      y += lineHeight;
      line = word;
    } else {
      line = candidate;
    }
    start = (spaceIdx == -1) ? (int)text.length() : spaceIdx + 1;
  }
  if (line.length() > 0) drawCenteredText(line, cx, y);
}

void renderDepartures(const DepartureResult& result, int yOffset, bool isTrain, int showLimit) {
  display.setFont(NULL);
  display.setTextSize(2);
  display.setTextColor(GxEPD_BLACK);

  if (result.statusMessage.length() > 0) {
    display.setCursor(startX, yOffset);
    display.print(result.statusMessage);
    return;
  }

  if (result.departures.empty()) {
    display.setCursor(startX, yOffset);
    display.print("No departures in the next 30 minutes");
    return;
  }

  int displayed = 0;
  for (const auto& dep : result.departures) {
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

} // namespace

void initDisplay(bool isWakeFromDeepSleep) {
  // GxEPD2's single-arg init() always passes initial=true to the panel driver,
  // which forces a full LUT re-init on every call - and per the library's own
  // docs, that overrides fast partial update regardless of setPartialWindow(),
  // silently turning every refresh into a full one. initial=false is only
  // correct when resuming from deep sleep with the panel's power rail kept up
  // the whole time (true here - nothing in this project gates its power), so
  // a genuine cold boot still gets the safe initial=true full init.
  display.init(115200, /*initial=*/!isWakeFromDeepSleep);
}

void drawBoard(const DepartureResult results[5], int batteryPercent, int hour24, int minute,
               bool useFastPartialUpdate) {
  display.setRotation(1);
  if (useFastPartialUpdate) {
    display.setPartialWindow(0, 0, display.width(), display.height());
  } else {
    display.setFullWindow();
  }
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
    renderDepartures(results[0], 90 + 45, true, 5);

    display.getTextBounds("Example Road (PR)", startX, 285 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Road (PR)", 285, false, t_w);
    renderDepartures(results[1], 285 + 45, false, 2);

    display.getTextBounds("Example Station (PA)", startX, 395 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Station (PA)", 395, false, t_w);
    renderDepartures(results[2], 395 + 45, false, 2);

    display.getTextBounds("Example Roundabout (AP)", startX, 505 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Roundabout (AP)", 505, false, t_w);
    renderDepartures(results[3], 505 + 45, false, 2);

    display.getTextBounds("Example Roundabout (AC)", startX, 615 + 15, &t_x1, &t_y1, &t_w, &t_h);
    drawBoxedSection("Example Roundabout (AC)", 615, false, t_w);
    renderDepartures(results[4], 615 + 45, false, 2);

    int footerY = 750;
    int footerH = 35;
    int16_t x1, y1; uint16_t w, h;
    char timeStr[20];
    sprintf(timeStr, "Updated: %02d:%02d", hour24, minute);
    display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    int timeBoxW = w + 20;
    display.drawRoundRect(startX, footerY, timeBoxW, footerH, 8, GxEPD_BLACK);
    display.setCursor(startX + 10, footerY + 10);
    display.print(timeStr);
    drawStickQueue(startX + 20, footerY);
    String batStr = "Battery: " + String(batteryPercent) + "%";
    display.getTextBounds(batStr, 0, 0, &x1, &y1, &w, &h);
    int batBoxW = w + 20;
    int batteryBoxX = 480 - batBoxW - 20;
    display.drawRoundRect(batteryBoxX, footerY, batBoxW, footerH, 8, GxEPD_BLACK);
    display.setCursor(batteryBoxX + 10, footerY + 10);
    display.print(batStr);
  } while (display.nextPage());
}

void drawStatusMessage(const String& line1, const String& line2, const uint8_t* icon196) {
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    if (icon196 != nullptr) {
      // Ported from Weather_Picture_Frame's drawError(): same icon bitmaps,
      // same font, same relative layout formula (icon centered, bottom edge
      // 21px above vertical-center; text starts 21px below vertical-center,
      // 55px between wrapped lines) - translated from their 800x480 canvas
      // to this panel's 480x800 (rotated) canvas, since the absolute
      // coordinates don't carry over between differently-shaped screens.
      const int cx = 480 / 2;
      const int cy = 800 / 2;
      display.drawInvertedBitmap(cx - 196 / 2, cy - 196 / 2 - 21, icon196, 196, 196, GxEPD_BLACK);
      display.setFont(&FreeSans_26pt8b);
      display.setTextSize(1);
      int textY = cy + 196 / 2 + 21;
      if (line2.length() > 0) {
        // Two explicit lines (each assumed to fit on its own line - callers
        // should keep these short given this panel's 480px width).
        drawCenteredText(line1, cx, textY);
        drawCenteredText(line2, cx, textY + 55);
      } else {
        drawWrappedCenteredText(line1, cx, textY, 400, 55);
      }
    } else {
      display.setFont(NULL);
      display.setTextSize(2);
      display.setCursor(20, 40);
      display.print(line1);
      if (line2.length() > 0) {
        display.setCursor(20, 70);
        display.print(line2);
      }
    }
  } while (display.nextPage());
}
