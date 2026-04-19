#include <Arduino.h>
#include <DMD32.h>
#include "fonts/SystemFont5x7.h"
#include "fonts/Arial_black_16.h"
#include "ScoreboardBLE.h"

//
// -----------------------------------------------------------------------------
// Choose display mode
// -----------------------------------------------------------------------------
//
// Real matrix:
// physical = 3 across x 7 down
// logical with rotate=true = 112 x 96
//
// For now keep this commented out as requested:
//
// #define USE_BIG_SCOREBOARD 1
//
// Testing on one 32x16 display:
#define USE_BIG_SCOREBOARD 0

#if USE_BIG_SCOREBOARD
  #define DISPLAYS_ACROSS 3
  #define DISPLAYS_DOWN   7
  DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN, true);
#else
  #define DISPLAYS_ACROSS 1
  #define DISPLAYS_DOWN   1
  DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN, false);
#endif

ScoreboardState gState;
ScoreboardBLE ble(gState);

unsigned long lastMarqueeStep = 0;
bool marqueeInitialized = false;

//
// -----------------------------------------------------------------------------
// Small text helpers
// -----------------------------------------------------------------------------
int textWidth(const uint8_t *font, const char *text) {
  dmd.selectFont(font);
  int w = 0;
  for (size_t i = 0; text[i] != '\0'; i++) {
    w += dmd.charWidth(text[i]);
    if (text[i + 1] != '\0') w += 1;
  }
  return w;
}

void drawText(int x, int y, const char *text, const uint8_t *font, byte mode = GRAPHICS_NORMAL) {
  dmd.selectFont(font);
  int cursorX = x;
  for (size_t i = 0; text[i] != '\0'; i++) {
    int cw = dmd.drawChar(cursorX, y, text[i], mode);
    if (cw > 0) cursorX += cw + 1;
  }
}

void drawCenteredText(int centerX, int y, const char *text, const uint8_t *font, byte mode = GRAPHICS_NORMAL) {
  int w = textWidth(font, text);
  drawText(centerX - (w / 2), y, text, font, mode);
}

void fillCircle(int cx, int cy, int r, byte mode = GRAPHICS_NORMAL) {
  for (int y = -r; y <= r; y++) {
    for (int x = -r; x <= r; x++) {
      if ((x * x + y * y) <= (r * r)) {
        dmd.writePixel(cx + x, cy + y, mode, true);
      }
    }
  }
}

void drawIndicator(bool filled, int cx, int cy, int r = 3) {
  if (filled) {
    fillCircle(cx, cy, r, GRAPHICS_NORMAL);
  } else {
    dmd.drawCircle(cx, cy, r, GRAPHICS_NORMAL);
  }
}

void drawIndicatorRow(char label, uint8_t value, uint8_t maxValue, int x, int y, int spacing) {
  char s[2] = { label, '\0' };
  drawText(x, y - 4, s, Arial_Black_16);

  int startX = x + 14;
  for (uint8_t i = 0; i < maxValue; i++) {
    drawIndicator(i < value, startX + i * spacing, y + 4, 4);
  }
}

void drawArrowUp(int cx, int cy) {
  dmd.drawLine(cx, cy - 5, cx - 4, cy + 3, GRAPHICS_NORMAL);
  dmd.drawLine(cx, cy - 5, cx + 4, cy + 3, GRAPHICS_NORMAL);
  dmd.drawLine(cx - 4, cy + 3, cx + 4, cy + 3, GRAPHICS_NORMAL);
}

void drawArrowDown(int cx, int cy) {
  dmd.drawLine(cx - 4, cy - 3, cx + 4, cy - 3, GRAPHICS_NORMAL);
  dmd.drawLine(cx - 4, cy - 3, cx, cy + 5, GRAPHICS_NORMAL);
  dmd.drawLine(cx + 4, cy - 3, cx, cy + 5, GRAPHICS_NORMAL);
}

//
// -----------------------------------------------------------------------------
// Big scoreboard renderer (112 x 96 logical) - currently not active
// -----------------------------------------------------------------------------
void renderBigScoreboard(const ScoreboardState &s) {
  dmd.setInvert(s.invert);
  dmd.clearScreen(true);

  const int W = 112;
  const int H = 96;

  dmd.drawBox(0, 0, W - 1, H - 1, GRAPHICS_NORMAL);

  // Top labels
  drawText(5, 4, "SCC", Arial_Black_16);
  drawCenteredText(W / 2, 6, "INN", System5x7);
  drawText(W - 30, 4, "GAST", System5x7);

  // Scores
  char homeBuf[3];
  char awayBuf[3];
  snprintf(homeBuf, sizeof(homeBuf), "%u", s.scoreHome);
  snprintf(awayBuf, sizeof(awayBuf), "%u", s.scoreAway);

  drawText(12, 24, homeBuf, Arial_Black_16);
  drawText(W - 28, 24, awayBuf, Arial_Black_16);

  // Inning + top/bottom
  char innBuf[3];
  snprintf(innBuf, sizeof(innBuf), "%u", s.inning);
  drawCenteredText(W / 2, 24, innBuf, Arial_Black_16);

  if (s.isTopFrame) {
    drawArrowUp(W / 2 - 12, 28);
  } else {
    drawArrowDown(W / 2 - 12, 28);
  }

  // Bottom indicators
  drawIndicatorRow('B', s.balls,   3, 6, 64, 10);
  drawIndicatorRow('S', s.strikes, 2, 72, 64, 10);
  drawIndicatorRow('O', s.outs,    2, 38, 82, 10);
}

//
// -----------------------------------------------------------------------------
// Test renderer for one 32x16 panel
// Just shows the values in a compact scrolling string.
// -----------------------------------------------------------------------------
void startTestMarquee(const ScoreboardState &s) {
  dmd.setInvert(s.invert);
  dmd.clearScreen(true);
  dmd.selectFont(System5x7);

  char buf[40];
  snprintf(
    buf,
    sizeof(buf),
    "%02u %02u %c%02u %u%u%u",
    s.scoreHome,
    s.scoreAway,
    s.isTopFrame ? 'T' : 'B',
    s.inning,
    s.balls,
    s.strikes,
    s.outs
  );

  dmd.drawMarquee(buf, strlen(buf), 31, 4);
  marqueeInitialized = true;
}

void stepTestMarquee() {
  if (!marqueeInitialized) return;

  unsigned long now = millis();
  if (now - lastMarqueeStep >= 50) {
    dmd.stepMarquee(-1, 0);
    lastMarqueeStep = now;
  }
}

//
// -----------------------------------------------------------------------------
// Unified render
// -----------------------------------------------------------------------------
void renderDisplay(const ScoreboardState &s) {
#if USE_BIG_SCOREBOARD
  renderBigScoreboard(s);
#else
  startTestMarquee(s);
#endif
}

//
// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  dmd.clearScreen(true);

  ble.begin("ESP32-Scoreboard");
  renderDisplay(gState);
}

//
// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------
void loop() {
  dmd.scanDisplayBySPI();

  if (ble.consumeDirty()) {
    renderDisplay(gState);
  }

#if !USE_BIG_SCOREBOARD
  stepTestMarquee();
#endif
}
