// Wander.h — Pip explores on his own, using the 8x8 ToF grid as sight.
//   SCAN  : turn in place, remember how open each direction is
//   TURN  : face the chosen direction (most open; or the nearest wall when sulking)
//   GO    : drive with eyes leading; slow near things
//   AVOID : stop, look left and right, turn toward the clearer side
//   INVEST: curious about an object: creep closer, tilt eyes, poke it with the forklift
//   EDGE  : table edge: peer over (EV_EDGE performance), back off, turn away
//   SULK  : drive up to a wall and stare at it
// Output is fwd/turn; Trackbot.ino feeds it to MotionEngine while `active`.
#pragma once
#include "config.h"
#include "Senses.h"
#include "Performances.h"

class Wander {
 public:
  bool active = false;
  float fwd = 0, turn = 0;

  // ms = how long this stroll lasts. sulk = go face a wall instead of exploring.
  void start(uint32_t ms, bool sulk_ = false) {
    if (!senses.tofOk) return;                 // can't wander blind
    active = true; sulk = sulk_; endAt = millis() + ms;
    enter(WN_SCAN);
  }
  void stop() { active = false; fwd = 0; turn = 0; st = WN_OFF; }

  void update(float dt) {
    if (!active) return;
    uint32_t now = millis(), in = now - stAt;
    float h = senses.imuOk ? senses.yaw : (est += (turn != 0 ? WANDER_TURN_DPS * dt : 0));
    turned += fabsf(h - lastH); lastH = h;
    float center = senses.zoneMean(2, 5, 1, 5);          // what's straight ahead

    switch (st) {
      case WN_SCAN: {
        fwd = 0; turn = WANDER_TURN * dir; eyes.lookAt(0.7f * dir, 0, 200);
        int bin = (int)(turned / 30.0f); if (bin > 11) bin = 11;
        if (center > bins[bin]) bins[bin] = center;
        if (turned >= 360 || in > 9000) {                // looked all around: choose
          int best = 0; float bestS = -1e9f;
          for (int i = 0; i < 12; i++) {
            if (bins[i] < 0) continue;
            float s = (sulk ? -bins[i] : bins[i]) + frand(0, 150);
            if (s > bestS) { bestS = s; best = i; }
          }
          target = best * 30 + 15; enter(WN_TURN);
        }
      } break;

      case WN_TURN:
        fwd = 0; turn = WANDER_TURN * dir; eyes.lookAt(0.7f * dir, 0, 200);
        if (turned >= target || in > 5000) enter(sulk ? WN_SULK : WN_GO);
        break;

      case WN_GO: {
        if (now > endAt) { stop(); return; }
        if (senses.edge) { enter(WN_EDGE); break; }       // Senses already fired EV_EDGE (peer + back up)
        if (senses.zoneCount(2, 5, 1, 5, WANDER_OBST_MM) >= 3) { obstacle(now); break; }   // something in the way
        float sp = WANDER_SPEED * (0.7f + 0.5f * brain.ring[M_ENERGY] / 100.0f);
        if (senses.zoneCount(2, 5, 1, 5, WANDER_OBST_MM * 2) >= 3) sp *= 0.6f;             // slow down near things
        fwd = sp; turn = 0.05f * sinf(now * 0.002f);      // tiny weave so it isn't robotic
        eyes.lookAt(0, 0.1f, 200);
      } break;

      case WN_AVOID:
        fwd = 0; turn = 0;
        if (in > 850) {                                   // after looking both ways
          float l = senses.zoneMean(0, 2, 1, 5), r = senses.zoneMean(5, 7, 1, 5);
          dir = r > l ? 1 : -1;                           // turn > 0 = turn right
          target = random(50, 110); enter(WN_TURN);
        }
        break;

      case WN_INVEST:
        fwd = 0; turn = 0;
        if (in > 2700) { if (motion.autoOK) motion.play(MV_BACKUP); enter(WN_AVOID); }
        break;

      case WN_EDGE:
        fwd = 0; turn = 0;
        if (in > 1900) { dir = random(2) ? 1 : -1; target = random(110, 180); enter(WN_TURN); }
        break;

      case WN_SULK:
        turn = 0; fwd = (senses.zoneCount(2, 5, 1, 5, 160) < 3 && in < 2500 && !senses.edge) ? WANDER_SPEED * 0.6f : 0;
        if (in > 2500 && now > nextSulk) { player.play(P_SULK, 1); nextSulk = now + 4500; }
        if (in > 14000) stop();
        break;

      default: break;
    }
  }

 private:
  enum St : uint8_t { WN_OFF, WN_SCAN, WN_TURN, WN_GO, WN_AVOID, WN_INVEST, WN_EDGE, WN_SULK };
  St st = WN_OFF;
  uint32_t stAt = 0, endAt = 0, lastInvest = 0, nextSulk = 0;
  float bins[12] = {0}, turned = 0, target = 0, lastH = 0, est = 0;
  int dir = 1; bool sulk = false;

  void enter(St s) {
    st = s; stAt = millis(); turned = 0;
    lastH = senses.imuOk ? senses.yaw : est;
    if (s == WN_SCAN) { for (float& b : bins) b = -1; dir = random(2) ? 1 : -1; }
  }

  void obstacle(uint32_t now) {
    fwd = 0; turn = 0;
    // a small close thing (not a wall) + a curious mood = go investigate it
    bool object = senses.closeCount(300) <= 26;
    if (object && brain.ring[M_CURIOUS] > 45 && now - lastInvest > 20000) {
      lastInvest = now; brain.ring[M_CURIOUS] = clampf(brain.ring[M_CURIOUS] - 20, 0, 100);
      player.play(P_INVESTIGATE, 1); enter(WN_INVEST);
    } else { player.play(P_AVOID_LOOK, 1); enter(WN_AVOID); }
  }
};

Wander wander;
