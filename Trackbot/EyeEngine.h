// EyeEngine.h — procedural Cozmo-style eyes (rounded rectangles) on the 128x64 OLED.
// A face is 8 numbers. Moods blend faces; performances can override for a moment.
// To add an expression: add an id to FaceId and a row to FACES.
#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// w,h = eye size px | r = corner radius | top = top-lid cover (0..1 of height)
// slope = lid slant (+ angry, - sad) | bot = bottom-lid/cheek lift (0..1, makes happy arcs)
// asym = left eye bigger than right (curious/smug) | y = vertical shift px
struct Face { float w, h, r, top, slope, bot, asym, y; };
#define FACE_N 8

enum FaceId : uint8_t {
  F_NEUTRAL, F_HAPPY, F_SAD, F_ANGRY, F_SLEEPY, F_CURIOUS, F_SCARED,   // mood faces (blended)
  F_JOY, F_SUSPICIOUS, F_WIDE, F_FOCUSED, F_PROUD, F_CLOSED, F_ANNOYED, F_PLEAD, F_SMUG,
  F_COUNT
};

const Face FACES[F_COUNT] = {
  /* NEUTRAL    */ {30, 34,  9, 0.00f,  0.0f, 0.00f, 0.00f,  0},
  /* HAPPY      */ {32, 30, 10, 0.00f,  0.0f, 0.35f, 0.00f, -2},
  /* SAD        */ {28, 28,  9, 0.28f, -0.6f, 0.00f, 0.00f,  3},
  /* ANGRY      */ {30, 26,  7, 0.30f,  0.7f, 0.10f, 0.00f,  1},
  /* SLEEPY     */ {30, 30,  9, 0.60f,  0.0f, 0.00f, 0.00f,  4},
  /* CURIOUS    */ {30, 36, 10, 0.00f,  0.0f, 0.00f, 0.25f, -2},
  /* SCARED     */ {34, 40, 12, 0.00f, -0.2f, 0.00f, 0.00f, -1},
  /* JOY        */ {32, 28, 10, 0.00f,  0.0f, 0.62f, 0.00f, -3},
  /* SUSPICIOUS */ {30, 22,  6, 0.42f,  0.2f, 0.25f, 0.25f,  0},
  /* WIDE       */ {36, 42, 13, 0.00f,  0.0f, 0.00f, 0.00f, -1},
  /* FOCUSED    */ {28, 24,  6, 0.20f,  0.25f, 0.15f, 0.00f, 0},
  /* PROUD      */ {30, 30,  9, 0.25f,  0.0f, 0.30f, 0.00f, -3},
  /* CLOSED     */ {32,  4,  2, 0.00f,  0.0f, 0.00f, 0.00f,  5},
  /* ANNOYED    */ {30, 24,  6, 0.42f,  0.0f, 0.00f, 0.00f,  0},
  /* PLEAD      */ {34, 38, 13, 0.10f, -0.35f, 0.00f, 0.00f, 2},
  /* SMUG       */ {30, 24,  8, 0.35f,  0.1f, 0.30f, 0.30f,  0},
};

// Mood weights the Brain feeds in (0..1 each), and which face each one pulls toward.
enum MoodW : uint8_t { W_HAPPY, W_SAD, W_ANGRY, W_SLEEPY, W_CURIOUS, W_SCARED, W_COUNT };
const FaceId MOOD_FACE[W_COUNT] = {F_HAPPY, F_SAD, F_ANGRY, F_SLEEPY, F_CURIOUS, F_SCARED};

enum Overlay : uint8_t { OV_NONE, OV_ZZZ, OV_HEARTS, OV_TEARS, OV_SPARKLE, OV_SPIRAL, OV_VEIN, OV_SWEAT, OV_NOTES };

static inline void faceLerp(Face& a, const Face& b, float t) {
  float* pa = (float*)&a; const float* pb = (const float*)&b;
  for (int i = 0; i < FACE_N; i++) pa[i] += (pb[i] - pa[i]) * t;
}

class EyeEngine {
 public:
  Adafruit_SSD1306 oled{128, 64, &Wire, -1};
  bool ok = false;

  bool begin() {
    ok = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    if (ok) { oled.clearDisplay(); oled.display(); }
    cur = FACES[F_CLOSED];           // boot with eyes shut; they open as he "wakes"
    nextBlink = millis() + 2500;
    return ok;
  }

  void setMood(const float* w) { for (int i = 0; i < W_COUNT; i++) moodW[i] = clampf(w[i], 0, 1); }
  // Temporary face from a performance. weight 1 = fully replace the mood face.
  void express(FaceId f, uint16_t ms, float weight = 1.0f) { exprFace = f; exprW = weight; exprEnd = millis() + ms; }
  // x,y in -1..1 (x + = Pip's right on screen, y + = down). Beats idle saccades for ms.
  void lookAt(float x, float y, uint16_t ms = 700) { gtx = clampf(x, -1, 1); gty = clampf(y, -1, 1); gazeHold = millis() + ms; }
  void setGravity(float x, float y) { gravX = x; gravY = y; }   // gravity eyes (from IMU tilt)
  void overlay(Overlay o, uint16_t ms) { ov = o; ovEnd = millis() + ms; }
  void blink() { if (blinkT < 0) blinkT = 0; }
  void bounce(float a = 1.0f) { bounceV = a; }                  // music/beat hop
  void setAsleep(bool s) { asleep = s; }
  // Focus timer screen (sleeping eyes + countdown). secLeft < 0 = back to the normal face.
  void setTimer(int32_t secLeft, int32_t total) { timerLeft = secLeft; timerTotal = total; }

  void update(float dt) {
    if (!ok) return;
    if (timerLeft >= 0) { drawTimer(); return; }
    uint32_t now = millis();

    // 1) target face = neutral + sum(mood weight * (mood face - neutral)), then overrides
    Face tgt = FACES[F_NEUTRAL];
    const float* n = (const float*)&FACES[F_NEUTRAL]; float* t = (float*)&tgt;
    for (int m = 0; m < W_COUNT; m++) {
      const float* f = (const float*)&FACES[MOOD_FACE[m]];
      for (int i = 0; i < FACE_N; i++) t[i] += moodW[m] * (f[i] - n[i]);
    }
    if (asleep) tgt = FACES[F_CLOSED];
    if (now < exprEnd) faceLerp(tgt, FACES[exprFace], exprW);
    faceLerp(cur, tgt, 1.0f - expf(-dt * EYE_EASE));   // ease, never snap

    // 2) blinks: irregular, sometimes double
    if (blinkT < 0 && !asleep && now > nextBlink) blinkT = 0;
    float bs = 1.0f;
    if (blinkT >= 0) {
      blinkT += dt;
      if (blinkT < 0.07f) bs = 1.0f - blinkT / 0.07f;
      else if (blinkT < 0.18f) bs = (blinkT - 0.07f) / 0.11f;
      else { blinkT = -1; nextBlink = now + (random(100) < 15 ? 220 : random(BLINK_MIN_MS, BLINK_MAX_MS)); }
    }

    // 3) gaze: held target, else idle saccades; gravity always adds on top
    if (now > gazeHold) {
      if (now > nextSacc) {
        if (random(100) < 35) { sx = 0; sy = 0; }
        else { sx = random(-60, 61) / 100.0f; sy = random(-30, 31) / 100.0f; }
        nextSacc = now + random(900, 3800);
      }
      gtx = sx; gty = sy;
    }
    if (asleep) { gtx = 0; gty = 0.3f; }
    float k = 1.0f - expf(-dt * GAZE_EASE);
    gx += (clampf(gtx + gravX, -1, 1) - gx) * k;
    gy += (clampf(gty + gravY, -1, 1) - gy) * k;
    bounceV *= expf(-dt * 7.0f);

    draw(bs);
  }

 private:
  Face cur = FACES[F_NEUTRAL];
  float moodW[W_COUNT] = {0};
  FaceId exprFace = F_NEUTRAL; float exprW = 0; uint32_t exprEnd = 0;
  float gx = 0, gy = 0, gtx = 0, gty = 0, sx = 0, sy = 0, gravX = 0, gravY = 0;
  uint32_t gazeHold = 0, nextSacc = 0, nextBlink = 0, ovEnd = 0;
  float blinkT = -1, bounceV = 0;
  Overlay ov = OV_NONE;
  bool asleep = false;
  int32_t timerLeft = -1, timerTotal = 1;

  void drawTimer() {
    oled.clearDisplay();
    float t = millis() * 0.001f; int br = (int)(1.5f * sinf(t * 0.9f));          // slow sleeping breath
    oled.fillRoundRect(38, 9 + br, 18, 3, 1, SSD1306_WHITE);                       // closed eyes
    oled.fillRoundRect(72, 9 + br, 18, 3, 1, SSD1306_WHITE);
    oled.setTextColor(SSD1306_WHITE); oled.setTextSize(1);
    float q = fmodf(t, 2.4f) / 2.4f; oled.setCursor((int)(100 + q * 14), (int)(12 - q * 10)); oled.print('z');
    char buf[16]; snprintf(buf, sizeof(buf), "%02d:%02d", (int)(timerLeft / 60), (int)(timerLeft % 60));
    oled.setTextSize(3); oled.setCursor(19, 22); oled.print(buf);                  // big MM:SS
    oled.drawRect(14, 54, 100, 6, SSD1306_WHITE);                                  // progress bar
    int w = timerTotal > 0 ? (int)(96.0f * (timerTotal - timerLeft) / timerTotal) : 0;
    oled.fillRect(16, 56, w < 0 ? 0 : (w > 96 ? 96 : w), 2, SSD1306_WHITE);
    oled.display();
  }

  void draw(float bs) {
    oled.clearDisplay();
    float tt = millis() * 0.001f;
    float breath = 1.0f + 0.03f * sinf(tt * 1.7f);       // slow breathing pulse
    if (ov != OV_NONE && millis() > ovEnd) ov = OV_NONE;
    for (int side = -1; side <= 1; side += 2) {           // -1 = left eye on screen
      float sc  = 1.0f + cur.asym * (side < 0 ? 0.5f : -0.5f);
      float par = 1.0f + 0.10f * gx * side;               // eye on the gaze side grows a bit
      float w = cur.w * sc * par, h = cur.h * sc * par * breath * bs;
      if (h < 3) h = 3;
      float cx = 64 + side * EYE_GAP + gx * 11, cy = 32 + cur.y + gy * 8 - bounceV * 6;
      if (ov == OV_SPIRAL) spiral(cx, cy, tt); else eye(cx, cy, w, h, -side);
    }
    overlays(tt);
    oled.display();
  }

  // inner = +1 when this eye's inner edge is on its right side
  void eye(float cx, float cy, float w, float h, int inner) {
    int iw = (int)w, ih = (int)h, x = (int)(cx - w / 2), y = (int)(cy - h / 2);
    int r = (int)cur.r; if (r > iw / 2) r = iw / 2; if (r > ih / 2) r = ih / 2;
    oled.fillRoundRect(x, y, iw, ih, r, SSD1306_WHITE);
    // top lid: black wedge, slanted by slope
    float base = cur.top * ih;
    int yo = (int)(y + base - cur.slope * ih * 0.25f), yi = (int)(y + base + cur.slope * ih * 0.35f);
    int yl = inner > 0 ? yo : yi, yr = inner > 0 ? yi : yo;
    if (yl > y || yr > y) {
      oled.fillTriangle(x - 2, y - 2, x + iw + 2, y - 2, x + iw + 2, yr, SSD1306_BLACK);
      oled.fillTriangle(x - 2, y - 2, x + iw + 2, yr, x - 2, yl, SSD1306_BLACK);
    }
    // bottom lid: big black circle pushed up from below = happy arc
    if (cur.bot > 0.02f) oled.fillCircle((int)cx, (int)(y + ih + iw - cur.bot * ih), iw, SSD1306_BLACK);
  }

  void spiral(float cx, float cy, float t) {              // dizzy eyes
    float px = cx, py = cy;
    for (float a = 0; a < 12.5f; a += 0.5f) {
      float rr = a * 1.25f, x = cx + cosf(a + t * 6) * rr, y = cy + sinf(a + t * 6) * rr;
      oled.drawLine((int)px, (int)py, (int)x, (int)y, SSD1306_WHITE);
      px = x; py = y;
    }
  }

  void drop(int x, int y) {
    oled.fillCircle(x, y, 2, SSD1306_WHITE);
    oled.fillTriangle(x - 2, y, x + 2, y, x, y - 5, SSD1306_WHITE);
  }
  void heart(int x, int y, int s) {
    oled.fillCircle(x - s / 2, y, s / 2 + 1, SSD1306_WHITE);
    oled.fillCircle(x + s / 2 + 1, y, s / 2 + 1, SSD1306_WHITE);
    oled.fillTriangle(x - s, y + 1, x + s + 1, y + 1, x, y + s + 2, SSD1306_WHITE);
  }

  void overlays(float t) {
    Overlay o = ov;
    if (asleep && o == OV_NONE) o = OV_ZZZ;
    float p = fmodf(t, 1.6f) / 1.6f;                       // 0..1 loop
    switch (o) {
      case OV_ZZZ:
        oled.setTextColor(SSD1306_WHITE);
        for (int i = 0; i < 3; i++) {
          float q = fmodf(p + i * 0.33f, 1.0f);
          oled.setTextSize(q > 0.6f ? 2 : 1);
          oled.setCursor((int)(92 + q * 22), (int)(28 - q * 26));
          oled.print('z');
        }
        break;
      case OV_HEARTS:
        for (int i = 0; i < 3; i++) { float q = fmodf(p + i * 0.33f, 1.0f); heart(10 + i * 54, (int)(58 - q * 50), 3 + (i & 1)); }
        break;
      case OV_TEARS:
        for (int s = -1; s <= 1; s += 2) { float q = fmodf(p + (s > 0 ? 0.4f : 0), 1.0f); drop(64 + s * (EYE_GAP + 8), (int)(46 + q * 18)); }
        break;
      case OV_SPARKLE: {
        const int8_t pts[4][2] = {{8, 10}, {118, 14}, {14, 52}, {112, 50}};
        for (int i = 0; i < 4; i++) if (((int)(t * 6) + i) & 1) {
          oled.drawFastHLine(pts[i][0] - 3, pts[i][1], 7, SSD1306_WHITE);
          oled.drawFastVLine(pts[i][0], pts[i][1] - 3, 7, SSD1306_WHITE);
        }
      } break;
      case OV_VEIN: {                                       // anime anger mark, top right
        int x = 114, y = 10, d = ((int)(t * 8) & 1);
        for (int sx2 = -1; sx2 <= 1; sx2 += 2) for (int sy2 = -1; sy2 <= 1; sy2 += 2) {
          oled.drawLine(x + sx2 * (2 + d), y + sy2 * 2, x + sx2 * (6 + d), y + sy2 * 2, SSD1306_WHITE);
          oled.drawLine(x + sx2 * (2 + d), y + sy2 * 2, x + sx2 * (2 + d), y + sy2 * 6, SSD1306_WHITE);
        }
      } break;
      case OV_SWEAT: drop(10, (int)(14 + p * 10)); break;
      case OV_NOTES:
        for (int i = 0; i < 2; i++) {
          int x = i ? 114 : 8, y = (int)(16 + 6 * sinf(t * 5 + i * 2));
          oled.fillCircle(x, y + 8, 2, SSD1306_WHITE);
          oled.drawFastVLine(x + 2, y, 8, SSD1306_WHITE);
          oled.drawFastHLine(x + 2, y, 4, SSD1306_WHITE);
        }
        break;
      default: break;
    }
  }
};

EyeEngine eyes;
