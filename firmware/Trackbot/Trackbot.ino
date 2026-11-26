// Trackbot.ino — Pip. Main loop wires the modules together; nothing here blocks.
// Module map:  config.h (pins/tunables) | EyeEngine | SoundEngine | GlowEngine | MotionEngine
//              Brain (moods) | Performances (reaction timelines) | Behaviors (picker) | Senses
//              Wander (explores using ToF) | Memory (flash + daily routine) | Net + WebApp
// Modes (Brain.h): AUTO = free to do whatever | MANUAL = only the phone moves him | FOLLOW | WANDER | SLEEP | FOCUS (timer nap)
// No hardware yet? Open Serial Monitor (115200) and type letters to fake events (see serialDebug()).
#include "config.h"
#include "EyeEngine.h"
#include "SoundEngine.h"
#include "GlowEngine.h"
#include "MotionEngine.h"
#include "Brain.h"
#include "Performances.h"
#include "Behaviors.h"
#include "Senses.h"
#include "Wander.h"
#include "Memory.h"
#if USE_WIFI
  #include "Net.h"
#endif

uint32_t lastUs = 0, lastMoodTx = 0, beatN = 0;
uint32_t focusEnd = 0, focusTotal = 0, stubbornUntil = 0, lastStubborn = 0, nextStroll = 30000;
bool wasWander = false;
bool wasConnected = false, wasMusic = false, wasManual = false, phewPending = false, wasJoy = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL); Wire.setClock(I2C_HZ);
#if USE_OLED
  if (!eyes.begin()) Serial.println("OLED not found");
#endif
#if USE_LED
  glow.begin();
#endif
  motion.begin();
#if USE_AUDIO
  sound.begin();
#endif
  memory.begin();              // load how he's been treated + days alive
  senses.begin();              // ToF firmware load happens here (eyes are still shut)
#if USE_WIFI
  netBegin();
#endif
  Serial.printf("Pip up. Day %d. OLED:%d IMU:%d ToF:%d\n", memory.days, eyes.ok, senses.imuOk, senses.tofOk);
  player.play(P_WAKE_GROGGY, 2);   // open eyes, yawn, stretch
  lastUs = micros();
}

// ---------- Focus timer: he naps with a countdown on his screen, then wakes you with a stretch ----------
void focusStop(bool finished) {
  if (brain.mode != MODE_FOCUS) return;
  eyes.setTimer(-1, 1);
  brain.asleep = false; brain.mode = MODE_AUTO;
  brain.event(finished ? EV_TIMER_DONE : EV_WAKE);
}
void focusStart(int minutes) {
  if (minutes <= 0) { focusStop(false); return; }
  wander.stop(); sound.musicStop();
  focusTotal = (uint32_t)minutes * 60; focusEnd = millis() + focusTotal * 1000UL;
  player.play(P_SLEEP, 3);
  brain.mode = MODE_FOCUS;
  memory.save();
}

// Type in Serial Monitor to test personality with no sensors attached.
void serialDebug() {
  if (!Serial.available()) return;
  switch (Serial.read()) {
    case 'p': brain.event(EV_PET); break;          case 't': brain.event(EV_TAP); break;
    case 'h': brain.event(EV_HAND_APPEAR); break;  case 'f': brain.event(EV_HAND_FAST); break;
    case 'l': brain.event(EV_HAND_LOST); break;    case 'u': brain.event(EV_PICKED_UP); break;
    case 's': brain.event(EV_SHAKEN); break;       case 'd': brain.event(EV_SETDOWN); break;
    case 'k': brain.event(EV_KNOCK); break;        case 'e': brain.event(EV_EDGE); break;
    case 'b': brain.event(EV_PEEKABOO); break;     case '5': brain.event(EV_HIGHFIVE); break;
    case 'z': brain.mode = MODE_SLEEP; break;      case 'w': wander.start(20000); break;
    case 'c': player.play(P_SNEEZE, 2); break;     case 'i': player.play(P_HICCUP, 2); break;
    case 'o': player.play(P_ZOOMIES, 2); break;    case '1': focusStart(1); break;
    case 'm': Serial.printf("hap %.0f sad %.0f ang %.0f en %.0f bored %.0f cur %.0f trust %.0f  mode %d day %d\n", brain.ring[0], brain.ring[1], brain.ring[2], brain.ring[3], brain.ring[4], brain.ring[5], brain.ring[6], brain.mode, memory.days); break;
    default: break;
  }
}

#if USE_WIFI
// Apply what the phone asked for, and send Pip's state back.
void handleApp() {
  ArduinoOTA.handle();
  uint32_t now = millis();
  bool connected = ws.count() > 0;
  if (connected != wasConnected) { wasConnected = connected; brain.event(connected ? EV_APP_CONNECT : EV_APP_DISCONNECT); }

  // joysticks (work in every mode except a focus nap)
  bool joy = now - app.lastJoy < MANUAL_HOLD_MS && (app.lx || app.ly || app.ry) && brain.mode != MODE_FOCUS;
  if (joy && !wasJoy && brain.mode != MODE_MANUAL && brain.ring[M_ANGRY] > STUBBORN_ANGER && now - lastStubborn > 15000) {
    lastStubborn = now; stubbornUntil = now + 1100;                         // stubborn streak: huff first, obey after
    player.play(P_STUBBORN, 2);
  }
  wasJoy = joy;
  if (joy && now >= stubbornUntil) {
    motion.manual(app.ly / 100.0f, app.lx / 100.0f);
    motion.armRate(app.ry / 100.0f);
    if (abs(app.lx) > 15) eyes.lookAt(app.lx / 100.0f, 0, 200);             // eyes lead the turn
  } else motion.manual(0, 0);

  if (app.mode >= 0) {
    int m = app.mode; app.mode = -1;
    if (m < MODE_COUNT && m != MODE_FOCUS) { if (brain.mode == MODE_FOCUS) focusStop(false); brain.mode = (Mode)m; wander.stop(); }
  }
  if (app.focusMin >= 0) { focusStart(app.focusMin); app.focusMin = -1; }
  if (app.gesture >= 0)  { motion.arm((ArmId)(int)app.gesture); app.gesture = -1; }
  if (app.party)         { app.party = false; if (brain.mode != MODE_FOCUS) { player.play(P_LAUGH, 2); motion.play(MV_SPIN); motion.arm(ARM_WAVE); } }
  if (app.resetMem)      { app.resetMem = false; memory.reset(); }
  if (app.dbgEvent >= 0) { if (app.dbgEvent < EV_COUNT) brain.event((Event)(int)app.dbgEvent); app.dbgEvent = -1; }
  if (app.dbgPerf >= 0)  { player.play((PerfId)(int)app.dbgPerf, 3); app.dbgPerf = -1; }

  // music: he dances to what he's playing
  bool music = app.lastMusic && now - app.lastMusic < 600 && brain.mode != MODE_FOCUS;
  if (music && !wasMusic) brain.event(EV_MUSIC_START);
  wasMusic = music;
  if (music) {
    brain.ring[M_BORED] = 0;
    eyes.express(app.bass > 150 ? F_JOY : F_HAPPY, 300, 0.8f);
    eyes.overlay(OV_NOTES, 400);
    if (app.beat) {
      app.beat = false; beatN++;
      eyes.bounce(1.0f); glow.pulse(1.0f);
      if (motion.autoOK && !motion.manualActive() && (beatN & 1)) motion.play(MV_BEAT);
      if (motion.autoOK && beatN % 8 == 0) motion.arm(ARM_BOB);
    }
  }

  if (connected && now - lastMoodTx > 200) {            // HUD, 5x per second
    lastMoodTx = now;
    long left = brain.mode == MODE_FOCUS ? ((long)(focusEnd - now) > 0 ? (long)(focusEnd - now) / 1000 : 0) : 0;
    char js[240];
    snprintf(js, sizeof(js), "{\"m\":[%d,%d,%d,%d,%d,%d],\"mood\":\"%s\",\"mode\":%d,\"zz\":%d,\"log\":\"%s\",\"seq\":%lu,\"day\":%d,\"focus\":%ld}",
             (int)brain.ring[M_HAPPY], (int)brain.ring[M_ENERGY], (int)brain.ring[M_TRUST],
             (int)brain.core[M_HAPPY], (int)brain.core[M_ENERGY], (int)brain.core[M_TRUST],
             brain.moodWord(), (int)brain.mode, (int)brain.asleep, brain.lastLog, (unsigned long)brain.logSeq, (int)memory.days, left);
    ws.textAll(js);
  }
  if (connected && app.tofView && senses.gridFresh) {   // ToF viewer
    senses.gridFresh = false;
    uint8_t pkt[129]; pkt[0] = 1; memcpy(pkt + 1, senses.grid, 128);
    ws.binaryAll(pkt, sizeof(pkt));
  }
  ws.cleanupClients();
}
#endif

// Decide when he goes for a stroll on his own, and feed Wander/Follow into the motors.
void autonomy(float dt, bool manual) {
  uint32_t now = millis();
  bool freeToMove = AUTO_MOTION && !brain.asleep && !manual && !wasMusic && !senses.held
                    && (brain.mode == MODE_AUTO || brain.mode == MODE_WANDER);
  if (wander.active && !freeToMove) wander.stop();

  if (!wander.active && freeToMove && senses.tofOk && !player.playing() && now > nextStroll) {
    float E = brain.ring[M_ENERGY];
    bool grumpy = brain.ring[M_ANGRY] > 55 || brain.ring[M_SAD] > 65;
    if (brain.mode == MODE_WANDER)            wander.start(20000);
    else if (grumpy && random(100) < 50)      wander.start(20000, true);              // sulking: go face a wall
    else if (brain.ring[M_BORED] > 35 && E > 25) wander.start(4000 + (uint32_t)(E * 120));   // low energy = short strolls
  }

  wander.update(dt);
  if (wasWander && !wander.active)                       // rest between strolls (Wander mode: short pause)
    nextStroll = now + (brain.mode == MODE_WANDER ? random(2000, 6000) : (uint32_t)(random(20000, 60000) * (1.6f - brain.ring[M_ENERGY] / 100.0f)));
  wasWander = wander.active;
  senses.obstacleMode = wander.active;
  senses.mirrorArm = !wander.active && !manual && brain.mode != MODE_MANUAL && brain.mode != MODE_FOCUS && !brain.asleep;
  if (wander.active) {
    motion.followOn = true; motion.followFwd = wander.fwd; motion.followTurn = wander.turn;
    brain.ring[M_BORED]  = clampf(brain.ring[M_BORED] - 3.0f * dt, 0, 100);          // exploring is fun
    brain.ring[M_ENERGY] = clampf(brain.ring[M_ENERGY] - 0.15f * dt, 0, 100);        // and tiring -> nap later
  } else {
    motion.followOn = (brain.mode == MODE_FOLLOW) && !brain.asleep;
    motion.followFwd = senses.folFwd; motion.followTurn = senses.folTurn;
  }
}

void loop() {
  uint32_t us = micros();
  float dt = (us - lastUs) / 1e6f; lastUs = us;
  if (dt > 0.1f) dt = 0.1f;

  serialDebug();
  senses.update(dt);
#if USE_WIFI
  handleApp();
#endif

  // sticks released for a while -> "phew", back to his own life
  bool manual = motion.manualActive();
  if (wasManual && !manual) phewPending = true;
  if (manual) phewPending = false;
  if (phewPending && motion.sinceManual() > MANUAL_PHEW_MS) { phewPending = false; if (brain.mode != MODE_MANUAL) brain.event(EV_STICKS_IDLE); }
  wasManual = manual;

  motion.autoOK = brain.mode != MODE_MANUAL && brain.mode != MODE_FOCUS;   // Manual: only the phone moves him
  brain.update(dt);
  memory.update();
  if (memory.morningDue && !brain.asleep && !player.playing() && brain.mode != MODE_FOCUS) { memory.morningDue = false; brain.event(EV_MORNING); }
  autonomy(dt, manual);

  // focus timer
  bool focus = brain.mode == MODE_FOCUS;
  if (focus) {
    long left = (long)(focusEnd - millis());
    if (left <= 0) focusStop(true);
    else if (!player.playing()) eyes.setTimer(left / 1000, focusTotal);    // countdown replaces the face
  }

  picker.update(!manual && !wasMusic && !wander.active && !focus);
  player.update();

  float w[W_COUNT]; brain.weights(w);
  eyes.setMood(w); eyes.setAsleep(brain.asleep && !player.playing());
#if USE_LED
  glow.setMood(w); glow.setAsleep(brain.asleep); glow.update(dt);
#endif
  motion.update(dt);
#if USE_OLED
  eyes.update(dt);             // draws the frame (~10-25 ms of I2C); keep last
#endif
}
