// MotionEngine.h — tracks (DRV8833, slow-decay PWM) + forklift servo.
// Priority: joystick > expressive micro-moves > follow controller. Edge block stops forward motion.
// To add a move: add an id + MV_x segment table + MOVES row. Same for arm gestures.
#pragma once
#include <ESP32Servo.h>
#include "config.h"

struct Seg { int8_t l, r; uint16_t ms; };            // track power in %, duration. {0,0,0} ends.
enum MoveId : uint8_t { MV_NONE, MV_NUDGE, MV_LEAN_IN, MV_BACKUP, MV_SCOOT_BACK, MV_STARTLE_BACK, MV_WIGGLE,
                        MV_SPIN, MV_TURN_AWAY, MV_SHAKE_NO, MV_PANIC, MV_BEAT, MV_TWITCH, MV_ZOOM, MV_HOP, MV_COUNT };
const Seg MVS_NUDGE[]   = {{35, 35, 120}, {0, 0, 0}};
const Seg MVS_LEAN[]    = {{40, 40, 180}, {0, 0, 150}, {-30, -30, 120}, {0, 0, 0}};
const Seg MVS_BACKUP[]  = {{-55, -55, 450}, {0, 0, 0}};
const Seg MVS_SCOOT[]   = {{-45, -45, 250}, {0, 0, 0}};
const Seg MVS_STARTLE[] = {{-90, -90, 180}, {0, 0, 0}};
const Seg MVS_WIGGLE[]  = {{50, -50, 110}, {-50, 50, 220}, {50, -50, 220}, {-50, 50, 110}, {0, 0, 0}};
const Seg MVS_SPIN[]    = {{70, -70, 900}, {0, 0, 0}};
const Seg MVS_AWAY[]    = {{55, -55, 550}, {0, 0, 0}};
const Seg MVS_NO[]      = {{45, -45, 90}, {-45, 45, 180}, {45, -45, 180}, {-45, 45, 90}, {0, 0, 0}};
const Seg MVS_PANIC[]   = {{90, 90, 150}, {-90, -90, 150}, {90, -90, 150}, {-90, 90, 150}, {90, 90, 150}, {-90, -90, 150}, {0, 0, 0}};
const Seg MVS_BEAT[]    = {{40, -40, 80}, {-40, 40, 80}, {0, 0, 0}};
const Seg MVS_TWITCH[]  = {{30, 0, 60}, {0, 0, 300}, {0, 30, 60}, {0, 0, 0}};
const Seg MVS_ZOOM[]    = {{80, -80, 500}, {85, 85, 350}, {-80, 80, 400}, {85, 85, 300}, {80, -80, 600}, {0, 0, 0}};   // zoomies
const Seg MVS_HOP[]     = {{-60, -60, 90}, {50, 50, 90}, {0, 0, 0}};                                                   // sneeze recoil
const Seg* const MOVES[MV_COUNT] = {nullptr, MVS_NUDGE, MVS_LEAN, MVS_BACKUP, MVS_SCOOT, MVS_STARTLE, MVS_WIGGLE,
                                    MVS_SPIN, MVS_AWAY, MVS_NO, MVS_PANIC, MVS_BEAT, MVS_TWITCH, MVS_ZOOM, MVS_HOP};

struct ArmSeg { uint8_t pos; uint16_t ms; };          // arm height 0..100 %, hold time. {0,0} ends.
enum ArmId : uint8_t { ARM_NONE, ARM_WAVE, ARM_NOD, ARM_SHRUG, ARM_FLEX, ARM_SHOW, ARM_DROOP, ARM_BUMP, ARM_BOB, ARM_HIGH5, ARM_COUNT };
const ArmSeg AS_WAVE[]  = {{80, 180}, {45, 180}, {80, 180}, {45, 180}, {10, 300}, {0, 0}};
const ArmSeg AS_NOD[]   = {{40, 200}, {5, 200}, {40, 200}, {5, 200}, {0, 0}};
const ArmSeg AS_SHRUG[] = {{60, 350}, {10, 300}, {0, 0}};
const ArmSeg AS_FLEX[]  = {{100, 500}, {70, 150}, {100, 300}, {10, 400}, {0, 0}};
const ArmSeg AS_SHOW[]  = {{100, 1200}, {0, 0}};
const ArmSeg AS_DROOP[] = {{0, 800}, {0, 0}};
const ArmSeg AS_BUMP[]  = {{70, 350}, {55, 120}, {10, 300}, {0, 0}};
const ArmSeg AS_BOB[]   = {{30, 150}, {8, 150}, {0, 0}};
const ArmSeg AS_HIGH5[] = {{100, 300}, {85, 100}, {100, 150}, {30, 300}, {0, 0}};
const ArmSeg* const ARMS[ARM_COUNT] = {nullptr, AS_WAVE, AS_NOD, AS_SHRUG, AS_FLEX, AS_SHOW, AS_DROOP, AS_BUMP, AS_BOB, AS_HIGH5};

class MotionEngine {
 public:
  bool autoOK = true;          // false in Manual mode: Pip never moves tracks or arm on his own
  bool edgeBlock = false;      // set by Senses: no forward driving
  bool lifted = false;         // set by Senses: no autonomous driving in the air (panic move excepted)
  float followFwd = 0, followTurn = 0; bool followOn = false;   // autonomous drive input (follow or wander)

  void begin() {
#if USE_MOTORS
    const int pins[4] = {PIN_MA1, PIN_MA2, PIN_MB1, PIN_MB2};
    for (int p : pins) { ledcAttach(p, MOTOR_PWM_HZ, 10); ledcWrite(p, 0); }
#endif
#if USE_SERVO
    servo.setPeriodHertz(50); servo.attach(PIN_SERVO, 500, 2400);
#endif
  }

  // Joystick: fwd/turn in -1..1. Call every packet.
  void manual(float fwd, float turn) { mFwd = fwd * tune.maxSpeed; mTurn = turn * tune.maxSpeed; if (fabsf(fwd) > 0.05f || fabsf(turn) > 0.05f) lastManual = millis(); }
  bool manualActive() const { return millis() - lastManual < MANUAL_HOLD_MS; }
  uint32_t sinceManual() const { return millis() - lastManual; }
  bool driving() const { return fabsf(curL) > 0.05f || fabsf(curR) > 0.05f; }

  void play(MoveId m) { if (m == MV_NONE || m >= MV_COUNT) return; seg = MOVES[m]; segT = millis(); force = (m == MV_PANIC); }
  void stopMove() { seg = nullptr; }

  void arm(ArmId g) { if (g == ARM_NONE || g >= ARM_COUNT) return; aseg = ARMS[g]; asegT = millis(); armT = aseg->pos / 100.0f; }
  void armRate(float r) { if (fabsf(r) < 0.05f) return; aseg = nullptr; armT = clampf(armT + r * ARM_RATE * lastDt, 0, 1); lastArmManual = millis(); }
  void armTo(float p) { aseg = nullptr; armT = clampf(p, 0, 1); }
  void armMirror(float p) { if (!aseg && autoOK) armT = clampf(p, 0, 1); }   // follow a hand; never interrupts a gesture
  float armPos() const { return armC; }

  void update(float dt) {
    lastDt = dt;
    float tl = 0, tr = 0;
    if (manualActive()) { tl = mFwd + mTurn; tr = mFwd - mTurn; seg = nullptr; }      // arcade mix
    else if (seg) {
      if (millis() - segT >= seg->ms) { seg++; segT = millis(); if (seg->ms == 0) seg = nullptr; }
      if (seg && autoOK && (force || (AUTO_MOTION && !lifted))) { tl = seg->l / 100.0f; tr = seg->r / 100.0f; }
    } else if (followOn && autoOK && AUTO_MOTION && !lifted) { tl = followFwd + followTurn; tr = followFwd - followTurn; }
    if (edgeBlock && !force && tl + tr > 0) { tl = 0; tr = 0; }                        // never drive off the table
    tl = clampf(tl, -1, 1); tr = clampf(tr, -1, 1);
    float step = MOTOR_RAMP * dt;
    curL += clampf(tl - curL, -step, step); curR += clampf(tr - curR, -step, step);
#if USE_MOTORS
    float a = MOTOR_SWAP ? curR : curL, b = MOTOR_SWAP ? curL : curR;
    setMotor(PIN_MA1, PIN_MA2, a * MOTOR_A_DIR); setMotor(PIN_MB1, PIN_MB2, b * MOTOR_B_DIR);
#endif
    // arm gesture timeline + easing
    if (aseg) {
      if (millis() - asegT >= aseg->ms) { aseg++; asegT = millis(); if (aseg->ms == 0) aseg = nullptr; else armT = aseg->pos / 100.0f; }
    }
    armC += (armT - armC) * (1.0f - expf(-dt * ARM_EASE));
#if USE_SERVO
    servo.write((int)(ARM_DOWN_DEG + armC * (ARM_UP_DEG - ARM_DOWN_DEG)));
#endif
  }

 private:
  Servo servo;
  float mFwd = 0, mTurn = 0, curL = 0, curR = 0, armT = 0.1f, armC = 0.1f, lastDt = 0.02f;
  uint32_t lastManual = 0x80000000u, lastArmManual = 0, segT = 0, asegT = 0;
  const Seg* seg = nullptr; const ArmSeg* aseg = nullptr; bool force = false;

  // DRV8833 slow decay: one pin held high, the other PWM'd (inverted). v in -1..1.
  void setMotor(int in1, int in2, float v) {
    float m = fabsf(v);
    if (m < 0.03f) { ledcWrite(in1, 0); ledcWrite(in2, 0); return; }          // coast
    int d = 1023 - (int)((MOTOR_MIN + (1.0f - MOTOR_MIN) * m) * 1023);
    if (v > 0) { ledcWrite(in1, 1023); ledcWrite(in2, d); }
    else       { ledcWrite(in1, d);    ledcWrite(in2, 1023); }
  }
};

MotionEngine motion;
