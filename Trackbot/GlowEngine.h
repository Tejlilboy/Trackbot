// GlowEngine.h — one WS2812B lighting the white shell from inside.
// Color = blend of mood colors, unless a performance sets a temporary color. Always fades.
#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "EyeEngine.h"   // MoodW

struct RGB { float r, g, b; };
enum GlowC : uint8_t { G_TEAL, G_YELLOW, G_BLUE, G_RED, G_DIMBLUE, G_CYAN, G_WHITE, G_PINK, G_PURPLE, G_COUNT };
const RGB GLOW_COLORS[G_COUNT] = {
  {60, 200, 170}, {255, 150, 20}, {20, 40, 160}, {255, 10, 0}, {8, 16, 80},
  {40, 200, 255}, {220, 220, 255}, {255, 70, 150}, {150, 40, 255},
};
// color each mood pulls toward (same order as MoodW): happy, sad, angry, sleepy, curious, scared
const GlowC MOOD_GLOW[W_COUNT] = {G_YELLOW, G_BLUE, G_RED, G_DIMBLUE, G_CYAN, G_WHITE};

class GlowEngine {
 public:
  Adafruit_NeoPixel px{1, PIN_LED, NEO_GRB + NEO_KHZ800};

  void begin() { px.begin(); px.show(); }
  void setMood(const float* w) { for (int i = 0; i < W_COUNT; i++) moodW[i] = clampf(w[i], 0, 1); angry = moodW[W_ANGRY]; }
  void flash(GlowC c, uint16_t ms) { ovC = c; ovEnd = millis() + ms; }   // temporary color
  void pulse(float a = 1.0f) { pulseV = a; }                              // beat / hit
  void setAsleep(bool s) { asleep = s; }

  void update(float dt) {
    RGB t = GLOW_COLORS[G_TEAL]; float sum = 0;
    for (int i = 0; i < W_COUNT; i++) sum += moodW[i];
    float keep = clampf(1.0f - sum, 0.15f, 1.0f);
    t = {t.r * keep, t.g * keep, t.b * keep};
    for (int i = 0; i < W_COUNT; i++) { const RGB& c = GLOW_COLORS[MOOD_GLOW[i]]; t.r += c.r * moodW[i]; t.g += c.g * moodW[i]; t.b += c.b * moodW[i]; }
    if (asleep) t = GLOW_COLORS[G_DIMBLUE];
    if (millis() < ovEnd) t = GLOW_COLORS[ovC];
    float k = 1.0f - expf(-dt * GLOW_EASE);
    cur.r += (t.r - cur.r) * k; cur.g += (t.g - cur.g) * k; cur.b += (t.b - cur.b) * k;

    float tt = millis() * 0.001f;
    float br = 0.8f + 0.2f * sinf(tt * (asleep ? 0.9f : 1.7f));        // breathing
    if (angry > 0.4f) br *= 0.6f + 0.4f * sinf(tt * 9.0f);             // angry pulse
    br += pulseV * 0.8f; pulseV *= expf(-dt * 9.0f);
    br = clampf(br, 0, 1.6f) * tune.glow * (asleep ? 0.4f : 1.0f);
    px.setPixelColor(0, px.Color((uint8_t)clampf(cur.r * br, 0, 255), (uint8_t)clampf(cur.g * br, 0, 255), (uint8_t)clampf(cur.b * br, 0, 255)));
    px.show();
  }

 private:
  float moodW[W_COUNT] = {0}, angry = 0, pulseV = 0;
  RGB cur = {0, 0, 0};
  GlowC ovC = G_TEAL; uint32_t ovEnd = 0;
  bool asleep = false;
};

GlowEngine glow;
