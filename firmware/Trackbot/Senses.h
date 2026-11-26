// Senses.h — turns raw sensors into events for the Brain, plus gaze/gravity for the eyes.
//   IMU (MPU6050, raw I2C): tilt -> gravity eyes; shaken, picked up, set down, upside down, knock, bump
//   Touch pad: tap / pet / over-pet
//   ToF (VL53L5CX 8x8): hand tracking, startle, peekaboo, fist bump, table edge, follow controller
// All thresholds are in config.h and need tuning on the real robot.
#pragma once
#include <Wire.h>
#include "config.h"
#include "Brain.h"
#include "EyeEngine.h"
#include "MotionEngine.h"
#if USE_TOF
  #include <SparkFun_VL53L5CX_Library.h>
#endif

class Senses {
 public:
  bool imuOk = false, tofOk = false;
  bool held = false, upside = false, touching = false;
  bool handSeen = false; float handCol = 3.5f, handRow = 3.5f, handDist = 0, handSpeed = 0;   // col 0 = Pip's left
  bool edge = false;
  float yaw = 0;                // deg, integrated gyro (drifts; only used for relative turns)
  float folFwd = 0, folTurn = 0; // follow-controller output
  bool obstacleMode = false;    // wandering: close things are obstacles, not hands to greet
  bool mirrorArm = false;       // forklift copies your hand height
  uint16_t grid[64] = {0};      // mm, row 0 = top; sent to the web ToF viewer
  bool gridFresh = false;

  void begin() {
#if USE_IMU
    Wire.beginTransmission(IMU_ADDR); Wire.write(0x6B); Wire.write(0);       // wake
    imuOk = Wire.endTransmission() == 0;
    if (imuOk) { reg(0x1C, 0x08); reg(0x1B, 0x08); reg(0x1A, 0x03); }        // +-4g, +-500dps, 44Hz filter
#endif
#if USE_TOUCH
    uint32_t sum = 0; for (int i = 0; i < 16; i++) { sum += touchRead(PIN_TOUCH); delay(5); }
    touchBase = sum / 16.0f;                                                  // don't touch him while booting
#endif
#if USE_TOF
    tofOk = tof.begin();                                                      // ~1-2 s firmware upload
    if (tofOk) { tof.setResolution(64); tof.setRangingFrequency(TOF_HZ); tof.startRanging(); }
#endif
  }

  // Wander's "sight": mean distance (mm) over a block of the grid; no reading = open space.
  float zoneMean(int c0, int c1, int r0, int r1) const {
    float s = 0; int n = 0;
    for (int r = r0; r <= r1; r++) for (int c = c0; c <= c1; c++) { uint16_t d = grid[r * 8 + c]; s += d ? d : 2000; n++; }
    return n ? s / n : 2000;
  }
  // How many zones in a block are closer than mm (catches small objects a mean would miss).
  int zoneCount(int c0, int c1, int r0, int r1, uint16_t mm) const {
    int n = 0;
    for (int r = r0; r <= r1; r++) for (int c = c0; c <= c1; c++) { uint16_t d = grid[r * 8 + c]; if (d && d < mm) n++; }
    return n;
  }
  int closeCount(uint16_t mm) const { int n = 0; for (int i = 0; i < 64 - TOF_FLOOR_ROWS * 8; i++) if (grid[i] && grid[i] < mm) n++; return n; }

  void update(float dt) {
    uint32_t now = millis();
#if USE_IMU
    if (imuOk && now - lastImu >= 10) { lastImu = now; imu(now); }
#endif
#if USE_TOUCH
    if (now - lastTouch >= 20) { lastTouch = now; touch(now); }
#endif
#if USE_TOF
    if (tofOk) tofUpdate(now);
#endif
    motion.lifted = held || upside;
    motion.edgeBlock = edge && !held;
    (void)dt;
  }

 private:
  // ---------------- IMU ----------------
  uint32_t lastImu = 0, heldSince = 0, stillSince = 0, upsideSince = 0, knockAt = 0, lastShake = 0, lastBump = 0;
  uint32_t spikes[SHAKE_COUNT] = {0}; uint8_t spikeI = 0;
  float motionLP = 0;

  void reg(uint8_t r, uint8_t v) { Wire.beginTransmission(IMU_ADDR); Wire.write(r); Wire.write(v); Wire.endTransmission(); }

  void imu(uint32_t now) {
    Wire.beginTransmission(IMU_ADDR); Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom((int)IMU_ADDR, 14) != 14) return;
    int16_t raw[7];
    for (int i = 0; i < 7; i++) { int hi = Wire.read(), lo = Wire.read(); raw[i] = (int16_t)((hi << 8) | lo); }
    float ax = raw[0] / 8192.0f, ay = raw[1] / 8192.0f, az = raw[2] / 8192.0f;         // g
    float gxr = raw[4] / 65.5f, gyr = raw[5] / 65.5f, gzr = raw[6] / 65.5f;            // deg/s
    float fwd = IMU_FWD(ax, ay, az), left = IMU_LEFT(ax, ay, az), up = IMU_UP(ax, ay, az);
    float dyn = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);                      // 0 when still
    float gmag = sqrtf(gxr * gxr + gyr * gyr + gzr * gzr);
    motionLP += (gmag - motionLP) * 0.15f;
    yaw += IMU_UP(gxr, gyr, gzr) * 0.01f;
    bool motors = motion.driving();

    // gravity eyes: pupils slide downhill
    eyes.setGravity(clampf(-left * GRAVITY_GAIN * GRAVITY_SIGN_X, -1, 1), clampf(fwd * GRAVITY_GAIN * GRAVITY_SIGN_Y, -1, 1));

    // shaken: several big spikes in a short window
    if (dyn > SHAKE_G) {
      spikes[spikeI] = now; spikeI = (spikeI + 1) % SHAKE_COUNT;
      uint32_t oldest = spikes[spikeI];
      if (oldest && now - oldest < SHAKE_WINDOW && now - lastShake > 3000) { lastShake = now; knockAt = 0; brain.event(EV_SHAKEN); }
    }
    // upside down
    if (up < -0.5f) { if (!upsideSince) upsideSince = now; if (!upside && now - upsideSince > 400) { upside = true; brain.event(EV_UPSIDE); } }
    else { upsideSince = 0; if (up > 0.6f) upside = false; }
    // picked up / set down (only judged while the motors are off)
    if (!held) {
      if (!motors && motionLP > HELD_DPS) { if (!heldSince) heldSince = now; if (now - heldSince > 350) { held = true; stillSince = 0; knockAt = 0; brain.event(EV_PICKED_UP); } }
      else heldSince = 0;
    } else {
      if (motionLP < SET_DPS && up > 0.8f) { if (!stillSince) stillSince = now; if (now - stillSince > 900) { held = false; heldSince = 0; brain.event(EV_SETDOWN); } }
      else stillSince = 0;
    }
    // knock: one sharp tap while sitting still (wait to be sure it isn't a pickup/shake)
    if (!held && !motors && dyn > KNOCK_G && motionLP < HELD_DPS && !knockAt) knockAt = now;
    if (knockAt && now - knockAt > 300) { if (!held && now - lastShake > 1000) brain.event(EV_KNOCK); knockAt = 0; }
    // bump: hit something while driving
    if (motors && dyn > BUMP_G && now - lastBump > 1500) { lastBump = now; brain.event(EV_BUMP); }
  }

  // ---------------- Touch ----------------
  uint32_t lastTouch = 0, touchStart = 0, lastPetTick = 0; float touchBase = 0; bool overpetted = false, petStarted = false;

  void touch(uint32_t now) {
    float v = touchRead(PIN_TOUCH);
    bool t = v > touchBase * TOUCH_RATIO;
    if (!t) touchBase += (v - touchBase) * 0.01f;            // slow drift tracking
    if (t && !touching) { touchStart = now; lastPetTick = now; overpetted = false; }
    if (t && touching) {
      uint32_t dur = now - touchStart;
      if (dur > OVERPET_MS && !overpetted) { overpetted = true; brain.event(EV_OVERPET); }
      else if (!overpetted && dur > PET_START_MS && (!petStarted || now - lastPetTick >= PET_TICK_MS)) { petStarted = true; lastPetTick = now; brain.event(EV_PET); }
    }
    if (!t) petStarted = false;
    if (!t && touching && now - touchStart < TAP_MS) brain.event(EV_TAP);
    touching = t;
  }

  // ---------------- ToF ----------------
#if USE_TOF
  SparkFun_VL53L5CX tof; VL53L5CX_ResultsData frame;
  uint32_t lastSeen = 0, firstSeen = 0, coverSince = 0, steadySince = 0, lastFist = 0, lastFrame = 0;
  bool covered = false, wasSeen = false; float floorBase = 0; uint8_t floorN = 0, edgeCount = 0; float lastTurnErr = 0;

  void tofUpdate(uint32_t now) {
    if (!tof.isDataReady() || !tof.getRangingData(&frame)) return;
    float fdt = lastFrame ? (now - lastFrame) / 1000.0f : 0.066f; lastFrame = now;

    // copy into grid[] with the mounting orientation fixed; 0 = no valid reading
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
      int rr = TOF_FLIP_Y ? 7 - r : r, cc = TOF_FLIP_X ? 7 - c : c;
      int src = TOF_TRANSPOSE ? cc * 8 + rr : rr * 8 + cc;
      uint8_t st = frame.target_status[src];
      grid[r * 8 + c] = (st == 5 || st == 9) ? (uint16_t)frame.distance_mm[src] : 0;
    }
    gridFresh = true;

    // hand = cluster of close zones above the floor rows. TODO: mask zones the forklift blocks (by motion.armPos()).
    float sc = 0, sr = 0, sd = 0; int n = 0, close = 0;
    for (int r = 0; r < 8 - TOF_FLOOR_ROWS; r++) for (int c = 0; c < 8; c++) {
      uint16_t d = grid[r * 8 + c];
      if (d && d < HAND_MAX_MM) { sc += c; sr += r; sd += d; n++; }
      if (d && d < COVER_MM) close++;
    }
    if (obstacleMode) { wasSeen = false; firstSeen = 0; }
    bool seen = n >= HAND_MIN_ZONES && !obstacleMode;
    if (seen) {
      float dist = sd / n;
      float sp = handSeen ? (handDist - dist) / fdt : 0;      // + = approaching, mm/s
      handSpeed += (sp - handSpeed) * 0.4f;
      handCol += (sc / n - handCol) * 0.5f; handRow += (sr / n - handRow) * 0.5f; handDist = dist;
      lastSeen = now;
      if (!wasSeen) { firstSeen = now; wasSeen = true; handCol = sc / n; handRow = sr / n; handSpeed = 0; steadySince = now; }
      // decide greeting vs startle a moment after it appears (needs a speed estimate)
      if (firstSeen && now - firstSeen > 200) { brain.event(handSpeed > HAND_FAST_MMS || dist < 70 ? EV_HAND_FAST : EV_HAND_APPEAR); firstSeen = 0; }
      eyes.lookAt((handCol - 3.5f) / 3.5f, (handRow - 3.5f) / 3.5f * 0.8f, 400);
      // arm mirroring: hand high -> forklift up
      if (mirrorArm && dist < MIRROR_MAX_MM) motion.armMirror((5.5f - handRow) / 5.0f);
      // steady + close: held high = high five, held lower = fist bump
      bool steady = dist > 40 && dist < 160 && fabsf(handSpeed) < 80;
      if (!steady) steadySince = now;
      else if (now - lastFist > 4000) {
        if (handRow < 1.8f && now - steadySince > 500) { lastFist = now; brain.event(EV_HIGHFIVE); }
        else if (handRow >= 2.5f && dist < 100 && now - steadySince > 1000) { lastFist = now; brain.event(EV_FISTBUMP); }
      }
    } else if (wasSeen && now - lastSeen > 1000) { wasSeen = false; firstSeen = 0; if (mirrorArm) motion.armMirror(0.1f); brain.event(EV_HAND_LOST); }
    handSeen = seen;

    // peekaboo: most of the view covered, then uncovered
    if (close > 30) { if (!coverSince) coverSince = now; if (!covered && now - coverSince > 400) { covered = true; brain.event(EV_COVERED); } }
    else { coverSince = 0; if (covered) { covered = false; brain.event(EV_PEEKABOO); } }

    // table edge: bottom row normally sees the table at a steady distance
    float fs = 0; int fn = 0;
    for (int c = 0; c < 8; c++) { uint16_t d = grid[56 + c]; if (d) { fs += d; fn++; } }
    float fl = fn ? fs / fn : 9999;
    if (floorN < 20) { if (fn >= 4) { floorBase += (fl - floorBase) / (++floorN); } }   // learn the table at boot
    else {
      bool gone = fn < 3 || fl > floorBase + EDGE_JUMP_MM;
      edgeCount = gone ? (edgeCount < 10 ? edgeCount + 1 : 10) : 0;
      bool e = edgeCount >= 2;
      if (e && !edge && !held && motion.driving()) brain.event(EV_EDGE);
      edge = e;
    }
    // follow controller (PD, no I). Used when brain.mode == MODE_FOLLOW.
    if (seen) {
      float te = handCol - 3.5f, de = handDist - FOLLOW_DIST_MM;
      float turn = tune.kpTurn * te + tune.kdTurn * (te - lastTurnErr); lastTurnErr = te;
      float fwd = tune.kpDist * de;
      if (fabsf(te) < 0.5f) turn = 0;
      if (fabsf(de) < 20) fwd = 0;
      folTurn = clampf(turn, -0.6f, 0.6f); folFwd = clampf(fwd, -0.6f, 0.6f);
    } else { folTurn = 0; folFwd = 0; }
  }
#endif
};

Senses senses;
