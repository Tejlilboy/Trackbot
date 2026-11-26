// Behaviors.h — the behavior picker. For each event, every matching row gets a score from
// Pip's current mood; the best one (plus a little randomness) plays. This is why the same
// input gets different reactions on different days.
// To add a behavior: add a row {event, performance, priority, cooldown s, score}.
#pragma once
#include "Brain.h"
#include "Performances.h"

#define SCORE [](const Brain& b) -> float
#define H b.ring[M_HAPPY]
#define S b.ring[M_SAD]
#define A b.ring[M_ANGRY]
#define E b.ring[M_ENERGY]
#define B b.ring[M_BORED]
#define C b.ring[M_CURIOUS]
#define T b.ring[M_TRUST]

struct Behavior { Event trig; PerfId perf; uint8_t prio; uint16_t coolS; float (*score)(const Brain&); };

const Behavior BEHAVIORS[] = {
  // hands
  {EV_HAND_APPEAR, P_GREET_CURIOUS,    1, 0, SCORE { return 30 + 0.4f * H + 0.3f * T - 0.8f * A; }},
  {EV_HAND_APPEAR, P_GREET_SUSPICIOUS, 1, 0, SCORE { return A + 0.5f * (60 - T) + (b.since(EV_SHAKEN) < 60000 ? 40 : 0); }},   // remembers a recent shake
  {EV_HAND_APPEAR, P_GREET_SLEEPY,     1, 0, SCORE { return 1.5f * (30 - E); }},
  {EV_HAND_FAST,   P_STARTLE,          2, 0, SCORE { return 50; }},
  {EV_HAND_FAST,   P_STARTLE_GLARE,    2, 0, SCORE { return 20 + A; }},
  {EV_HAND_LOST,   P_HAND_LOST_SAD,    1, 8, SCORE { return 15 + 0.4f * H + 0.3f * T - A; }},
  {EV_HAND_LOST,   P_HAND_LOST_SHRUG,  1, 8, SCORE { return 30; }},
  {EV_COVERED,     P_PEEK_CONFUSED,    1, 3, SCORE { return 40; }},
  {EV_PEEKABOO,    P_LAUGH,            1, 0, SCORE { return 30 + 0.5f * H - 0.6f * A; }},
  {EV_PEEKABOO,    P_PEEK_HUFF,        1, 0, SCORE { return 10 + A; }},
  {EV_FISTBUMP,    P_FISTBUMP,         1, 5, SCORE { return 40 + 0.3f * H - A; }},
  // touch
  {EV_TAP,         P_TAP_HM,           1, 0, SCORE { return 40 + 0.3f * H - A; }},
  {EV_TAP,         P_TAP_GRUMBLE,      1, 0, SCORE { return 10 + A + (30 - E); }},
  {EV_PET,         P_PET_PURR,         1, 0, SCORE { return 50 + 0.3f * T - 2 * A; }},
  {EV_PET,         P_PET_SOFTEN,       1, 0, SCORE { return 60 - fabsf(A - 35) * 2; }},
  {EV_PET,         P_PET_STIFF,        1, 0, SCORE { return A * 1.2f + (b.since(EV_SHAKEN) < 12000 ? 30 : 0); }},
  {EV_OVERPET,     P_OVERPET,          1, 0, SCORE { return 50; }},
  // motion
  {EV_PICKED_UP,   P_PICKUP_WHEE,      2, 0, SCORE { return 30 + 0.4f * H + 0.4f * T - A; }},
  {EV_PICKED_UP,   P_PICKUP_PANIC,     2, 0, SCORE { return 30 + 0.6f * (60 - T) + 0.5f * A; }},
  {EV_SHAKEN,      P_SHAKEN_DIZZY,     2, 0, SCORE { return 60 - 0.5f * A; }},
  {EV_SHAKEN,      P_SHAKEN_ANGRY,     2, 0, SCORE { return 0.9f * A; }},
  {EV_UPSIDE,      P_UPSIDE,           2, 0, SCORE { return 50; }},
  {EV_KNOCK,       P_KNOCK_WHO,        1, 2, SCORE { return 40 + 0.2f * C - 0.6f * A; }},
  {EV_KNOCK,       P_KNOCK_GRUMBLE,    1, 2, SCORE { return A + (30 - E); }},
  {EV_SETDOWN,     P_SETDOWN,          1, 0, SCORE { return 40; }},
  {EV_EDGE,        P_EDGE,             3, 2, SCORE { return 90; }},
  {EV_BUMP,        P_BUMP_OOPS,        1, 2, SCORE { return 40; }},
  // phone / play
  {EV_APP_CONNECT,    P_APP_HELLO,     1, 10, SCORE { return 40 + 0.3f * H - A; }},
  {EV_APP_CONNECT,    P_APP_GLANCE,    1, 10, SCORE { return A + (30 - E); }},
  {EV_APP_DISCONNECT, P_APP_BYE,       1, 10, SCORE { return 20 + 0.4f * H + 0.2f * T - 0.5f * A; }},
  {EV_STICKS_IDLE, P_PHEW,             0, 20, SCORE { return 40; }},
  {EV_MUSIC_START, P_MUSIC_START,      1, 5, SCORE { return 50; }},
  {EV_HIGHFIVE,    P_HIGHFIVE,         1, 3, SCORE { return 40 + 0.3f * H - A; }},
  {EV_MORNING,     P_MORNING,          2, 0, SCORE { return 50; }},
  {EV_TIMER_DONE,  P_TIMER_DONE,       3, 0, SCORE { return 90; }},
  // sleep
  {EV_SLEEP,       P_SLEEP,            2, 0, SCORE { return 50; }},
  {EV_WAKE,        P_WAKE_GROGGY,      2, 0, SCORE { return 40 - 0.5f * A; }},
  {EV_WAKE,        P_WAKE_GRUMPY,      2, 0, SCORE { return 10 + A; }},
  {EV_DREAM,       P_DREAM,            0, 12, SCORE { return 30; }},
  // idle life (picked every few seconds when nothing is happening)
  {EV_IDLE, P_IDLE_LOOK,       0, 10,  SCORE { return 20 + 0.2f * B; }},
  {EV_IDLE, P_IDLE_SIGH,       0, 20,  SCORE { return 0.5f * S + 0.3f * B - 15; }},
  {EV_IDLE, P_IDLE_FIDGET,     0, 15,  SCORE { return 0.4f * B + 0.2f * E - 10; }},
  {EV_IDLE, P_IDLE_BABBLE,     0, 25,  SCORE { return 0.3f * H + 0.2f * E; }},
  {EV_IDLE, P_IDLE_TRUSTBLINK, 0, 60,  SCORE { return 0.4f * T - 10; }},
  {EV_IDLE, P_IDLE_WIGGLE,     0, 30,  SCORE { return 0.5f * H + 0.3f * E - 30; }},
  {EV_IDLE, P_IDLE_MRRP,       0, 25,  SCORE { return 0.3f * H; }},
  {EV_IDLE, P_SMUG,            0, 120, SCORE { return 0.4f * H - 10; }},
  {EV_IDLE, P_YAWN,            0, 40,  SCORE { return 1.2f * (45 - E); }},
  {EV_IDLE, P_LONELY,          0, 90,  SCORE { return 0.8f * S + 0.5f * B - 50; }},
  // quirks: rare and random on purpose
  {EV_IDLE, P_SNEEZE,          0, 120, SCORE { (void)b; return random(100) < 4 ? 80.0f : -100.0f; }},
  {EV_IDLE, P_HICCUP,          0, 300, SCORE { (void)b; return random(100) < 3 ? 80.0f : -100.0f; }},
  {EV_IDLE, P_ZOOMIES,         0, 180, SCORE { return (E > 70 && H > 60) ? 0.6f * E + 0.3f * H - 40 : -100.0f; }},
};
#define N_BEHAVIORS (sizeof(BEHAVIORS) / sizeof(Behavior))

#undef H
#undef S
#undef A
#undef E
#undef B
#undef C
#undef T

class Picker {
 public:
  // React to one event. Returns true if a performance started.
  bool react(Event e) {
    int best = -1; float bestScore = 0; uint32_t now = millis();
    for (unsigned i = 0; i < N_BEHAVIORS; i++) {
      const Behavior& bh = BEHAVIORS[i];
      if (bh.trig != e) continue;
      if (lastUsed[i] && now - lastUsed[i] < bh.coolS * 1000UL) continue;
      float s = bh.score(brain) + frand(0, 18);           // randomness = never fully predictable
      if (s > bestScore) { bestScore = s; best = (int)i; }
    }
    if (best < 0) return false;
    if (!player.play(BEHAVIORS[best].perf, BEHAVIORS[best].prio)) return false;
    lastUsed[best] = now ? now : 1;
    return true;
  }

  // Call every loop. allowIdle = false while driving manually / music playing.
  void update(bool allowIdle) {
    Event e;
    while (brain.pop(e)) react(e);
    if (!allowIdle || player.playing() || sound.busy()) { nextIdle = millis() + random(1500, 3500); return; }
    if (millis() > nextIdle) {
      react(brain.asleep ? EV_DREAM : EV_IDLE);
      float lazy = 1.5f - brain.ring[M_ENERGY] / 100.0f;  // low energy = longer pauses
      nextIdle = millis() + (uint32_t)(random(IDLE_MIN_MS, IDLE_MAX_MS) * lazy);
    }
  }

 private:
  uint32_t lastUsed[N_BEHAVIORS] = {0}, nextIdle = 6000;
};

Picker picker;
