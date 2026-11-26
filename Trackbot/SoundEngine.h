// SoundEngine.h — Pip's voice. Three sources mixed in a background task:
//   1) procedural chirps (note tables below, randomized every time)
//   2) samples stored in flash (R2 clips, 8 kHz 8-bit, any playback speed)
//   3) streamed music from the phone (16 kHz 8-bit ring buffer)
// Output: I2S PDM on one pin -> PAM8302. No ISR, nothing blocks the main loop.
// To add a chirp: add an id + NOTES_x table + SOUNDS row.
// To add a sample: drop sound_xxx.h in the sketch folder, add it to SAMPLES below.
#pragma once
#include <ESP_I2S.h>
#include "config.h"

// Sample headers are optional (copyrighted, git-ignored). Missing = procedural fallback.
#if __has_include("sound_r2_laugh.h")
  #include "sound_r2_laugh.h"
  #define HAVE_R2_LAUGH 1
#endif
#if __has_include("sound_r2_lonely.h")
  #include "sound_r2_lonely.h"
  #define HAVE_R2_LONELY 1
#endif
#if __has_include("sound_r2_startled.h")
  #include "sound_r2_startled.h"
  #define HAVE_R2_STARTLED 1
#endif

enum Wave : uint8_t { WV_SIN, WV_TRI, WV_SQR };
// f0->f1 sweep in Hz over ms; vib = pitch wobble; trem = volume flutter; gap = silence after (ms)
struct Note { uint16_t f0, f1, ms; uint8_t wave, vibHz, vibPct, tremHz, gap; };

enum SoundId : uint8_t {
  S_HM, S_TRILL, S_SAD, S_PURR, S_STARTLE, S_GRUMBLE, S_YAWN, S_HUFF, S_MRRP, S_GIGGLE,
  S_PHEW, S_WHEE, S_OOPS, S_WHO, S_SNORE, S_OK, S_DIZZY, S_ANGRY, S_SNEEZE, S_HIC,
  S_PROC_COUNT,
  S_R2_LAUGH = S_PROC_COUNT, S_R2_LONELY, S_R2_STARTLED,   // samples
  S_COUNT
};

const Note NOTES_HM[]      = {{500, 950, 160, WV_SIN, 0, 0, 0, 0}};
const Note NOTES_TRILL[]   = {{1400,1400,45,WV_SIN,0,0,0,10},{1800,1800,45,WV_SIN,0,0,0,10},{1400,1400,45,WV_SIN,0,0,0,10},
                              {1800,1800,45,WV_SIN,0,0,0,10},{1500,1500,45,WV_SIN,0,0,0,10},{2000,2200,80,WV_SIN,0,0,0,0}};
const Note NOTES_SAD[]     = {{700, 330, 520, WV_SIN, 6, 3, 0, 0}};
const Note NOTES_PURR[]    = {{190, 180, 1100, WV_TRI, 0, 0, 24, 0}};
const Note NOTES_STARTLE[] = {{1500, 2600, 70, WV_SIN, 0, 0, 0, 0}, {2400, 900, 160, WV_SIN, 0, 0, 0, 0}};
const Note NOTES_GRUMBLE[] = {{230, 190, 300, WV_SQR, 9, 6, 0, 40}, {210, 170, 260, WV_SQR, 9, 6, 0, 0}};
const Note NOTES_YAWN[]    = {{500, 900, 350, WV_SIN, 0, 0, 0, 0}, {900, 350, 700, WV_SIN, 4, 3, 0, 0}};
const Note NOTES_HUFF[]    = {{420, 260, 140, WV_SQR, 0, 0, 0, 0}};
const Note NOTES_MRRP[]    = {{600, 1100, 90, WV_TRI, 30, 8, 0, 0}, {1100, 1300, 110, WV_TRI, 30, 8, 0, 0}};
const Note NOTES_GIGGLE[]  = {{1300,1500,55,WV_SIN,0,0,0,40},{1200,1400,55,WV_SIN,0,0,0,40},{1350,1550,55,WV_SIN,0,0,0,40},
                              {1150,1350,55,WV_SIN,0,0,0,40},{1300,1700,90,WV_SIN,0,0,0,0}};
const Note NOTES_PHEW[]    = {{900, 400, 380, WV_SIN, 0, 0, 0, 0}};
const Note NOTES_WHEE[]    = {{700, 2200, 420, WV_SIN, 7, 4, 0, 0}};
const Note NOTES_OOPS[]    = {{900, 700, 90, WV_SIN, 0, 0, 0, 30}, {600, 450, 160, WV_SIN, 0, 0, 0, 0}};
const Note NOTES_WHO[]     = {{600, 800, 120, WV_SIN, 0, 0, 0, 60}, {800, 1200, 180, WV_SIN, 0, 0, 0, 0}};
const Note NOTES_SNORE[]   = {{160, 210, 600, WV_TRI, 0, 0, 18, 200}, {900, 700, 300, WV_SIN, 0, 0, 0, 0}};
const Note NOTES_OK[]      = {{1000, 1000, 60, WV_SIN, 0, 0, 0, 20}, {1500, 1500, 90, WV_SIN, 0, 0, 0, 0}};
const Note NOTES_DIZZY[]   = {{900, 600, 900, WV_SIN, 5, 18, 0, 0}};
const Note NOTES_ANGRY[]   = {{300, 250, 180, WV_SQR, 14, 10, 0, 30}, {350, 220, 300, WV_SQR, 14, 10, 0, 0}};

const Note NOTES_SNEEZE[]  = {{700, 1100, 220, WV_SIN, 12, 4, 0, 60}, {2600, 500, 130, WV_SQR, 0, 0, 0, 0}};   // "ah... CHOO"
const Note NOTES_HIC[]     = {{900, 1500, 55, WV_SIN, 0, 0, 0, 0}};

struct SoundDef { const Note* n; uint8_t count; };
#define SND(x) {x, (uint8_t)(sizeof(x) / sizeof(Note))}
const SoundDef SOUNDS[S_PROC_COUNT] = {   // order must match SoundId
  SND(NOTES_HM), SND(NOTES_TRILL), SND(NOTES_SAD), SND(NOTES_PURR), SND(NOTES_STARTLE), SND(NOTES_GRUMBLE),
  SND(NOTES_YAWN), SND(NOTES_HUFF), SND(NOTES_MRRP), SND(NOTES_GIGGLE), SND(NOTES_PHEW), SND(NOTES_WHEE),
  SND(NOTES_OOPS), SND(NOTES_WHO), SND(NOTES_SNORE), SND(NOTES_OK), SND(NOTES_DIZZY), SND(NOTES_ANGRY),
  SND(NOTES_SNEEZE), SND(NOTES_HIC),
};

struct Sample { const uint8_t* d; uint32_t len, rate; SoundId fallback; };
const Sample SAMPLES[S_COUNT - S_PROC_COUNT] = {   // order must match the sample ids
#ifdef HAVE_R2_LAUGH
  {R2_LAUGH, R2_LAUGH_LEN, R2_LAUGH_RATE, S_GIGGLE},
#else
  {nullptr, 0, 8000, S_GIGGLE},
#endif
#ifdef HAVE_R2_LONELY
  {R2_LONELY, R2_LONELY_LEN, R2_LONELY_RATE, S_SAD},
#else
  {nullptr, 0, 8000, S_SAD},
#endif
#ifdef HAVE_R2_STARTLED
  {R2_STARTLED, R2_STARTLED_LEN, R2_STARTLED_RATE, S_STARTLE},
#else
  {nullptr, 0, 8000, S_STARTLE},
#endif
};

#define MAX_NOTES 8
struct Voice { Note n[MAX_NOTES]; uint8_t count = 0; float pitch = 1, speed = 1; };

class SoundEngine {
 public:
  bool ok = false;

  void begin() {
    // PDM: data pin only. If begin() fails on your core version, give it a clock pin: setPinsPdmTx(PIN_SPARE, PIN_SPK)
    i2s.setPinsPdmTx(-1, PIN_SPK);
    ok = i2s.begin(I2S_MODE_PDM_TX, AUDIO_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    if (ok) xTaskCreatePinnedToCore(taskFn, "audio", 4096, this, 5, nullptr, 1);
  }

  // pitch/speed are multipliers; small random variation is added so he never repeats exactly.
  void play(SoundId id, float pitch = 1.0f, float speed = 1.0f) {
    pitch *= frand(0.94f, 1.06f); speed *= frand(0.92f, 1.08f);
    if (id >= S_PROC_COUNT) {
      const Sample& s = SAMPLES[id - S_PROC_COUNT];
      if (!s.d) { id = s.fallback; }
      else {
        portENTER_CRITICAL(&mux);
        pSmp = s.d; pSmpLen = s.len;
        pSmpStep = (uint32_t)(65536.0f * s.rate / AUDIO_RATE * pitch);   // faster = higher
        smpFlag = true;
        portEXIT_CRITICAL(&mux);
        return;
      }
    }
    Voice v; const SoundDef& d = SOUNDS[id];
    v.count = d.count > MAX_NOTES ? MAX_NOTES : d.count;
    for (int i = 0; i < v.count; i++) v.n[i] = d.n[i];
    v.pitch = pitch; v.speed = speed;
    submit(v);
  }

  // Random "talking": happy 0..1 raises pitch, energy 0..1 makes it faster/longer.
  void babble(float happy, float energy) {
    Voice v; v.count = 3 + random(4);
    float base = 500 + happy * 700;
    for (int i = 0; i < v.count; i++) {
      float f0 = base * frand(0.7f, 1.4f), f1 = f0 * frand(0.8f, 1.3f);
      v.n[i] = {(uint16_t)f0, (uint16_t)f1, (uint16_t)random(50, 130), (uint8_t)(random(3) ? WV_SIN : WV_TRI), 0, 0, 0, (uint8_t)random(15, 55)};
    }
    v.speed = 0.8f + energy * 0.5f;
    submit(v);
  }

  bool busy() const { return active; }

  // Streamed music from the phone (called from the WebSocket task).
  void musicPush(const uint8_t* d, size_t n) {
    for (size_t i = 0; i < n; i++) {
      uint32_t next = (mHead + 1) % MUSIC_BUF;
      if (next == mTail) return;       // full: drop the rest
      ring[mHead] = d[i]; mHead = next;
    }
  }
  void musicStop() { mTail = mHead; }

 private:
  I2SClass i2s;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  Voice pend; volatile bool pendFlag = false;
  const uint8_t* pSmp = nullptr; uint32_t pSmpLen = 0, pSmpStep = 0; volatile bool smpFlag = false;
  volatile bool active = false;
  uint8_t ring[MUSIC_BUF]; volatile uint32_t mHead = 0, mTail = 0; bool mPlaying = false;

  // audio-task state
  Voice cur; uint8_t vi = 255; uint32_t pos = 0, nSamp = 0, gSamp = 0; float ph = 0;
  const uint8_t* sd = nullptr; uint32_t slen = 0, sstep = 0, spos = 0;

  void submit(const Voice& v) { portENTER_CRITICAL(&mux); pend = v; pendFlag = true; portEXIT_CRITICAL(&mux); }

  void startNote() {
    const Note& n = cur.n[vi];
    nSamp = (uint32_t)(n.ms * (AUDIO_RATE / 1000.0f) / cur.speed); if (nSamp < 8) nSamp = 8;
    gSamp = (uint32_t)(n.gap * (AUDIO_RATE / 1000.0f) / cur.speed);
    pos = 0;
  }

  inline float synth() {
    if (vi >= cur.count) return 0;
    const Note& n = cur.n[vi];
    float out = 0;
    if (pos < nSamp) {
      float u = (float)pos / nSamp, ts = (float)pos / AUDIO_RATE;
      float f = (n.f0 + ((float)n.f1 - (float)n.f0) * u) * cur.pitch;
      if (n.vibHz) f *= 1.0f + n.vibPct * 0.01f * sinf(6.2832f * n.vibHz * ts);
      ph += f / AUDIO_RATE; if (ph >= 1) ph -= 1;
      float v = n.wave == WV_SIN ? sinf(6.2832f * ph) : n.wave == WV_TRI ? (ph < 0.5f ? 4 * ph - 1 : 3 - 4 * ph) : (ph < 0.5f ? 0.6f : -0.6f);
      float a = 1; const uint32_t att = AUDIO_RATE / 250, rel = AUDIO_RATE / 125;   // click-free edges
      if (pos < att) a = (float)pos / att; else if (nSamp - pos < rel) a = (float)(nSamp - pos) / rel;
      if (n.tremHz) a *= 0.5f + 0.5f * sinf(6.2832f * n.tremHz * ts);
      out = v * a;
    }
    if (++pos >= nSamp + gSamp) { vi++; if (vi < cur.count) startNote(); }
    return out;
  }

  static void taskFn(void* p) { ((SoundEngine*)p)->run(); }

  void run() {
    static int16_t buf[AUDIO_BLOCK];
    for (;;) {
      if (pendFlag) { portENTER_CRITICAL(&mux); cur = pend; pendFlag = false; portEXIT_CRITICAL(&mux); vi = 0; ph = 0; startNote(); }
      if (smpFlag)  { portENTER_CRITICAL(&mux); sd = pSmp; slen = pSmpLen; sstep = pSmpStep; smpFlag = false; portEXIT_CRITICAL(&mux); spos = 0; }
      uint32_t fill = (mHead + MUSIC_BUF - mTail) % MUSIC_BUF;
      if (!mPlaying && fill > MUSIC_PREBUF) mPlaying = true;
      if (fill == 0) mPlaying = false;
      for (int i = 0; i < AUDIO_BLOCK; i++) {
        float s = synth() * 0.55f;
        if (sd) {
          uint32_t idx = spos >> 16;
          if (idx + 1 >= slen) sd = nullptr;
          else {
            int a = (int)sd[idx] - 128, b = (int)sd[idx + 1] - 128;
            s += (a + (b - a) * ((spos & 0xFFFF) / 65536.0f)) / 128.0f * 0.9f;
            spos += sstep;
          }
        }
        if (mPlaying && mTail != mHead) { s += ((int)ring[mTail] - 128) / 128.0f * tune.musicVol; mTail = (mTail + 1) % MUSIC_BUF; }
        s = clampf(s * tune.volume, -1, 1);
        buf[i] = (int16_t)(s * 30000);
      }
      active = (vi < cur.count) || sd;
      i2s.write((uint8_t*)buf, sizeof(buf));   // blocks until the DMA buffer has room = our pacing
    }
  }
};

SoundEngine sound;
