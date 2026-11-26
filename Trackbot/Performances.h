// Performances.h — a performance = one timeline firing eyes + sound + glow + motion + arm together.
// This file is DATA: to add a reaction, add its name to PERF_LIST and write its PF_ table.
// Step = {time ms, type, a, b}. Last step must be {total_ms, ST_END}.
#pragma once
#include "EyeEngine.h"
#include "SoundEngine.h"
#include "GlowEngine.h"
#include "MotionEngine.h"
#include "Brain.h"

enum StepType : uint8_t {
  ST_END,
  ST_FACE,    // a = FaceId,  b = hold ms
  ST_SND,     // a = SoundId, b = pitch % (0 = 100)
  ST_BABBLE,  // mood-shaped random chatter
  ST_GLOW,    // a = GlowC,   b = ms
  ST_MOVE,    // a = MoveId
  ST_ARM,     // a = ArmId
  ST_OV,      // a = Overlay, b = ms
  ST_GAZE,    // a = x*100, b = y*100 (-100..100)
  ST_BLINK,
  ST_BOUNCE,  // little eye hop
};
struct Step { uint16_t t; uint8_t type; int16_t a, b; };

#define PERF_LIST(X) \
  X(GREET_CURIOUS) X(GREET_SUSPICIOUS) X(GREET_SLEEPY) X(STARTLE) X(STARTLE_GLARE) \
  X(HAND_LOST_SAD) X(HAND_LOST_SHRUG) X(TAP_HM) X(TAP_GRUMBLE) X(PET_PURR) X(PET_SOFTEN) X(PET_STIFF) X(OVERPET) \
  X(PICKUP_WHEE) X(PICKUP_PANIC) X(SHAKEN_DIZZY) X(SHAKEN_ANGRY) X(UPSIDE) X(KNOCK_WHO) X(KNOCK_GRUMBLE) \
  X(SETDOWN) X(EDGE) X(BUMP_OOPS) X(APP_HELLO) X(APP_GLANCE) X(APP_BYE) X(PEEK_CONFUSED) X(PEEK_HUFF) X(LAUGH) \
  X(FISTBUMP) X(HIGHFIVE) X(PHEW) X(PROUD) X(MUSIC_START) X(STUBBORN) X(TIMER_DONE) X(MORNING) \
  X(SNEEZE) X(HICCUP) X(ZOOMIES) X(INVESTIGATE) X(AVOID_LOOK) X(SULK) \
  X(IDLE_LOOK) X(IDLE_SIGH) X(IDLE_FIDGET) X(IDLE_BABBLE) X(IDLE_TRUSTBLINK) X(IDLE_WIGGLE) X(IDLE_MRRP) X(SMUG) \
  X(YAWN) X(LONELY) X(SLEEP) X(DREAM) X(WAKE_GROGGY) X(WAKE_GRUMPY)

enum PerfId : uint8_t {
#define X(n) P_##n,
  PERF_LIST(X)
#undef X
  P_COUNT
};

// ---- hands ----
const Step PF_GREET_CURIOUS[]    = {{0,ST_FACE,F_WIDE,300},{0,ST_GLOW,G_CYAN,1500},{150,ST_SND,S_HM,100},{150,ST_FACE,F_CURIOUS,1200},{200,ST_MOVE,MV_LEAN_IN,0},{700,ST_SND,S_TRILL,100},{700,ST_FACE,F_HAPPY,700},{1500,ST_END,0,0}};
const Step PF_GREET_SUSPICIOUS[] = {{0,ST_FACE,F_SUSPICIOUS,1800},{0,ST_GLOW,G_RED,1200},{200,ST_SND,S_GRUMBLE,100},{400,ST_MOVE,MV_BACKUP,0},{1800,ST_END,0,0}};
const Step PF_GREET_SLEEPY[]     = {{0,ST_FACE,F_ANNOYED,900},{0,ST_GAZE,0,-30},{300,ST_SND,S_HUFF,90},{1000,ST_END,0,0}};
const Step PF_STARTLE[]          = {{0,ST_FACE,F_WIDE,500},{0,ST_SND,S_STARTLE,100},{0,ST_GLOW,G_WHITE,400},{0,ST_OV,OV_SWEAT,900},{30,ST_MOVE,MV_STARTLE_BACK,0},{500,ST_FACE,F_SCARED,600},{1200,ST_END,0,0}};
const Step PF_STARTLE_GLARE[]    = {{0,ST_FACE,F_WIDE,450},{0,ST_SND,S_STARTLE,100},{0,ST_GLOW,G_WHITE,400},{30,ST_MOVE,MV_STARTLE_BACK,0},{500,ST_FACE,F_ANGRY,1300},{600,ST_SND,S_ANGRY,100},{1800,ST_END,0,0}};
const Step PF_HAND_LOST_SAD[]    = {{0,ST_FACE,F_PLEAD,1200},{0,ST_GAZE,-60,0},{350,ST_GAZE,60,0},{700,ST_SND,S_SAD,100},{700,ST_FACE,F_SAD,900},{1700,ST_END,0,0}};
const Step PF_HAND_LOST_SHRUG[]  = {{0,ST_GAZE,-50,0},{400,ST_GAZE,50,0},{800,ST_ARM,ARM_SHRUG,0},{800,ST_SND,S_HM,80},{1500,ST_END,0,0}};
const Step PF_PEEK_CONFUSED[]    = {{0,ST_FACE,F_CURIOUS,1200},{0,ST_SND,S_HM,90},{0,ST_GAZE,-60,0},{500,ST_GAZE,60,0},{1200,ST_END,0,0}};
const Step PF_PEEK_HUFF[]        = {{0,ST_FACE,F_ANNOYED,900},{0,ST_SND,S_HUFF,100},{900,ST_END,0,0}};
const Step PF_LAUGH[]            = {{0,ST_FACE,F_JOY,1900},{0,ST_SND,S_R2_LAUGH,105},{0,ST_GLOW,G_YELLOW,2000},{0,ST_OV,OV_SPARKLE,1800},{200,ST_MOVE,MV_WIGGLE,0},{1900,ST_END,0,0}};
const Step PF_FISTBUMP[]         = {{0,ST_FACE,F_FOCUSED,500},{0,ST_ARM,ARM_BUMP,0},{500,ST_SND,S_OK,100},{500,ST_FACE,F_PROUD,900},{1500,ST_END,0,0}};
// ---- touch ----
const Step PF_TAP_HM[]           = {{0,ST_FACE,F_CURIOUS,700},{0,ST_GAZE,0,-80},{50,ST_SND,S_HM,110},{800,ST_END,0,0}};
const Step PF_TAP_GRUMBLE[]      = {{0,ST_FACE,F_ANNOYED,900},{0,ST_SND,S_GRUMBLE,100},{900,ST_END,0,0}};
const Step PF_PET_PURR[]         = {{0,ST_FACE,F_JOY,1400},{0,ST_GLOW,G_PINK,1600},{0,ST_SND,S_PURR,100},{300,ST_MOVE,MV_NUDGE,0},{1150,ST_END,0,0}};
const Step PF_PET_SOFTEN[]       = {{0,ST_FACE,F_HAPPY,1300},{0,ST_GLOW,G_PINK,1200},{300,ST_SND,S_PURR,85},{1150,ST_END,0,0}};
const Step PF_PET_STIFF[]        = {{0,ST_FACE,F_ANNOYED,1300},{0,ST_GAZE,70,0},{600,ST_SND,S_HUFF,90},{1150,ST_END,0,0}};
const Step PF_OVERPET[]          = {{0,ST_FACE,F_ANNOYED,1200},{0,ST_SND,S_HUFF,100},{200,ST_MOVE,MV_SCOOT_BACK,0},{600,ST_MOVE,MV_SHAKE_NO,0},{1300,ST_END,0,0}};
// ---- motion (IMU) ----
const Step PF_PICKUP_WHEE[]      = {{0,ST_FACE,F_WIDE,400},{0,ST_SND,S_STARTLE,110},{0,ST_GLOW,G_WHITE,500},{500,ST_FACE,F_JOY,1200},{600,ST_SND,S_WHEE,100},{1700,ST_END,0,0}};
const Step PF_PICKUP_PANIC[]     = {{0,ST_FACE,F_WIDE,900},{0,ST_SND,S_R2_STARTLED,115},{0,ST_GLOW,G_WHITE,600},{0,ST_OV,OV_SWEAT,2000},{50,ST_MOVE,MV_PANIC,0},{900,ST_FACE,F_SCARED,1500},{2500,ST_END,0,0}};
const Step PF_SHAKEN_DIZZY[]     = {{0,ST_OV,OV_SPIRAL,2200},{0,ST_SND,S_DIZZY,100},{0,ST_GLOW,G_PURPLE,2000},{2200,ST_FACE,F_ANNOYED,900},{2300,ST_MOVE,MV_SHAKE_NO,0},{2400,ST_SND,S_HUFF,100},{3200,ST_END,0,0}};
const Step PF_SHAKEN_ANGRY[]     = {{0,ST_OV,OV_SPIRAL,1500},{0,ST_SND,S_DIZZY,100},{1500,ST_FACE,F_ANGRY,2000},{1500,ST_OV,OV_VEIN,2000},{1500,ST_GLOW,G_RED,2500},{1600,ST_SND,S_ANGRY,100},{2200,ST_MOVE,MV_TURN_AWAY,0},{3600,ST_END,0,0}};
const Step PF_UPSIDE[]           = {{0,ST_FACE,F_SCARED,2000},{0,ST_SND,S_R2_STARTLED,120},{0,ST_OV,OV_SWEAT,2000},{0,ST_GLOW,G_WHITE,2000},{60,ST_MOVE,MV_PANIC,0},{2200,ST_END,0,0}};
const Step PF_KNOCK_WHO[]        = {{0,ST_FACE,F_WIDE,300},{100,ST_SND,S_WHO,100},{200,ST_GAZE,-80,0},{600,ST_GAZE,80,0},{1000,ST_FACE,F_CURIOUS,600},{1600,ST_END,0,0}};
const Step PF_KNOCK_GRUMBLE[]    = {{0,ST_FACE,F_ANNOYED,1000},{0,ST_SND,S_GRUMBLE,100},{1000,ST_END,0,0}};
const Step PF_SETDOWN[]          = {{0,ST_FACE,F_HAPPY,900},{0,ST_SND,S_PHEW,100},{200,ST_BLINK,0,0},{1000,ST_END,0,0}};
const Step PF_EDGE[]             = {{0,ST_FACE,F_SCARED,1500},{0,ST_SND,S_OOPS,100},{0,ST_GAZE,0,90},{0,ST_OV,OV_SWEAT,1500},{300,ST_MOVE,MV_BACKUP,0},{1600,ST_END,0,0}};
const Step PF_BUMP_OOPS[]        = {{0,ST_FACE,F_WIDE,400},{0,ST_SND,S_OOPS,100},{200,ST_MOVE,MV_BACKUP,0},{500,ST_FACE,F_PLEAD,700},{1300,ST_END,0,0}};
// ---- phone / play ----
const Step PF_APP_HELLO[]        = {{0,ST_FACE,F_JOY,1200},{0,ST_SND,S_MRRP,100},{0,ST_GLOW,G_YELLOW,1500},{300,ST_ARM,ARM_WAVE,0},{400,ST_MOVE,MV_WIGGLE,0},{1500,ST_END,0,0}};
const Step PF_APP_GLANCE[]       = {{0,ST_GAZE,0,-60},{0,ST_FACE,F_ANNOYED,700},{800,ST_END,0,0}};
const Step PF_APP_BYE[]          = {{0,ST_FACE,F_PLEAD,1500},{0,ST_GLOW,G_BLUE,2000},{200,ST_SND,S_R2_LONELY,100},{400,ST_ARM,ARM_DROOP,0},{1800,ST_END,0,0}};
const Step PF_PHEW[]             = {{0,ST_SND,S_PHEW,100},{0,ST_FACE,F_HAPPY,700},{100,ST_BLINK,0,0},{800,ST_END,0,0}};
const Step PF_PROUD[]            = {{0,ST_FACE,F_PROUD,1800},{0,ST_SND,S_TRILL,90},{0,ST_OV,OV_SPARKLE,1500},{200,ST_ARM,ARM_SHOW,0},{1800,ST_END,0,0}};
const Step PF_MUSIC_START[]      = {{0,ST_FACE,F_JOY,1200},{0,ST_OV,OV_NOTES,3000},{0,ST_ARM,ARM_FLEX,0},{1200,ST_END,0,0}};
const Step PF_HIGHFIVE[]         = {{0,ST_FACE,F_JOY,1200},{0,ST_ARM,ARM_HIGH5,0},{350,ST_SND,S_OK,120},{350,ST_GLOW,G_YELLOW,800},{400,ST_OV,OV_SPARKLE,900},{1300,ST_END,0,0}};
const Step PF_STUBBORN[]         = {{0,ST_FACE,F_ANNOYED,1100},{0,ST_SND,S_HUFF,85},{0,ST_GAZE,80,0},{0,ST_OV,OV_VEIN,900},{1100,ST_END,0,0}};
const Step PF_TIMER_DONE[]       = {{0,ST_FACE,F_SLEEPY,700},{200,ST_BLINK,0,0},{300,ST_SND,S_YAWN,110},{500,ST_ARM,ARM_FLEX,0},{1300,ST_FACE,F_JOY,1500},{1300,ST_SND,S_TRILL,100},{1300,ST_GLOW,G_YELLOW,2500},{1500,ST_ARM,ARM_WAVE,0},{1500,ST_OV,OV_SPARKLE,1500},{3000,ST_END,0,0}};
const Step PF_MORNING[]          = {{0,ST_FACE,F_SLEEPY,800},{300,ST_SND,S_YAWN,110},{500,ST_ARM,ARM_FLEX,0},{1400,ST_FACE,F_HAPPY,1200},{1400,ST_SND,S_MRRP,100},{1600,ST_MOVE,MV_WIGGLE,0},{2600,ST_END,0,0}};
// ---- quirks ----
const Step PF_SNEEZE[]           = {{0,ST_FACE,F_SUSPICIOUS,400},{0,ST_SND,S_SNEEZE,100},{0,ST_GAZE,0,-60},{280,ST_FACE,F_CLOSED,250},{280,ST_MOVE,MV_HOP,0},{600,ST_FACE,F_WIDE,400},{700,ST_BLINK,0,0},{1200,ST_END,0,0}};
const Step PF_HICCUP[]           = {{0,ST_SND,S_HIC,100},{0,ST_BOUNCE,0,0},{0,ST_FACE,F_WIDE,150},{700,ST_SND,S_HIC,100},{700,ST_BOUNCE,0,0},{700,ST_FACE,F_WIDE,150},{1500,ST_SND,S_HIC,110},{1500,ST_BOUNCE,0,0},{1700,ST_FACE,F_ANNOYED,600},{2400,ST_END,0,0}};
const Step PF_ZOOMIES[]          = {{0,ST_FACE,F_JOY,2400},{0,ST_SND,S_WHEE,100},{0,ST_OV,OV_SPARKLE,2200},{100,ST_MOVE,MV_ZOOM,0},{1200,ST_SND,S_TRILL,115},{2300,ST_SND,S_PHEW,100},{2600,ST_END,0,0}};
// ---- wander ----
const Step PF_INVESTIGATE[]      = {{0,ST_FACE,F_CURIOUS,2600},{0,ST_SND,S_HM,105},{0,ST_GLOW,G_CYAN,2500},{300,ST_MOVE,MV_NUDGE,0},{700,ST_MOVE,MV_NUDGE,0},{1200,ST_ARM,ARM_BUMP,0},{1500,ST_SND,S_OK,100},{1900,ST_FACE,F_HAPPY,600},{2600,ST_END,0,0}};
const Step PF_AVOID_LOOK[]       = {{0,ST_FACE,F_FOCUSED,700},{0,ST_GAZE,-90,0},{350,ST_GAZE,90,0},{700,ST_GAZE,0,0},{800,ST_END,0,0}};
const Step PF_SULK[]             = {{0,ST_FACE,F_ANNOYED,3500},{0,ST_GAZE,0,40},{300,ST_SND,S_HUFF,85},{500,ST_ARM,ARM_DROOP,0},{3500,ST_END,0,0}};
// ---- idle life ----
const Step PF_IDLE_LOOK[]        = {{0,ST_GAZE,-80,-10},{700,ST_GAZE,80,-10},{1400,ST_GAZE,0,0},{1500,ST_END,0,0}};
const Step PF_IDLE_SIGH[]        = {{0,ST_FACE,F_SAD,1500},{100,ST_SND,S_SAD,80},{300,ST_ARM,ARM_DROOP,0},{1700,ST_END,0,0}};
const Step PF_IDLE_FIDGET[]      = {{0,ST_ARM,ARM_BOB,0},{0,ST_GAZE,0,60},{200,ST_FACE,F_FOCUSED,1000},{1300,ST_END,0,0}};
const Step PF_IDLE_BABBLE[]      = {{0,ST_BABBLE,0,0},{0,ST_FACE,F_CURIOUS,700},{900,ST_END,0,0}};
const Step PF_IDLE_TRUSTBLINK[]  = {{0,ST_FACE,F_SLEEPY,400},{400,ST_FACE,F_CLOSED,350},{900,ST_FACE,F_HAPPY,500},{1500,ST_END,0,0}};   // cat "I trust you"
const Step PF_IDLE_WIGGLE[]      = {{0,ST_FACE,F_HAPPY,900},{0,ST_SND,S_TRILL,100},{100,ST_MOVE,MV_WIGGLE,0},{1100,ST_END,0,0}};
const Step PF_IDLE_MRRP[]        = {{0,ST_SND,S_MRRP,100},{0,ST_FACE,F_CURIOUS,600},{700,ST_END,0,0}};
const Step PF_SMUG[]             = {{0,ST_FACE,F_SMUG,1500},{300,ST_SND,S_GIGGLE,100},{1600,ST_END,0,0}};
const Step PF_YAWN[]             = {{0,ST_FACE,F_SLEEPY,400},{300,ST_FACE,F_CLOSED,900},{300,ST_SND,S_YAWN,100},{400,ST_ARM,ARM_FLEX,0},{1500,ST_FACE,F_SLEEPY,800},{2300,ST_END,0,0}};
const Step PF_LONELY[]           = {{0,ST_FACE,F_PLEAD,2200},{0,ST_GAZE,0,50},{0,ST_GLOW,G_BLUE,2500},{200,ST_SND,S_R2_LONELY,90},{600,ST_ARM,ARM_DROOP,0},{1200,ST_MOVE,MV_TURN_AWAY,0},{2400,ST_END,0,0}};
const Step PF_SLEEP[]            = {{0,ST_FACE,F_SLEEPY,1200},{0,ST_SND,S_YAWN,85},{0,ST_GLOW,G_DIMBLUE,3000},{300,ST_ARM,ARM_DROOP,0},{1500,ST_END,0,0}};
const Step PF_DREAM[]            = {{0,ST_SND,S_SNORE,100},{600,ST_MOVE,MV_TWITCH,0},{1400,ST_END,0,0}};
const Step PF_WAKE_GROGGY[]      = {{0,ST_FACE,F_SLEEPY,1200},{200,ST_BLINK,0,0},{500,ST_BLINK,0,0},{600,ST_SND,S_YAWN,115},{900,ST_ARM,ARM_FLEX,0},{1800,ST_END,0,0}};
const Step PF_WAKE_GRUMPY[]      = {{0,ST_FACE,F_ANNOYED,1800},{0,ST_OV,OV_VEIN,1200},{100,ST_SND,S_GRUMBLE,100},{1800,ST_END,0,0}};

const Step* const PERFS[P_COUNT] = {
#define X(n) PF_##n,
  PERF_LIST(X)
#undef X
};

class Player {
 public:
  PerfId id = P_COUNT;
  bool playing() const { return cur != nullptr; }

  // prio: 0 idle, 1 reaction, 2 urgent (edge, shaken). Lower prio can't interrupt higher.
  bool play(PerfId p, uint8_t pr = 1) {
    if (p >= P_COUNT || (cur && pr < prio)) return false;
    cur = PERFS[p]; idx = 0; t0 = millis(); prio = pr; id = p;
    return true;
  }

  void update() {
    if (!cur) return;
    uint32_t t = millis() - t0;
    while (cur[idx].type != ST_END && cur[idx].t <= t) exec(cur[idx++]);
    if (cur[idx].type == ST_END && t >= cur[idx].t) { cur = nullptr; prio = 0; }
  }

 private:
  const Step* cur = nullptr; uint8_t idx = 0, prio = 0; uint32_t t0 = 0;

  void exec(const Step& s) {
    switch (s.type) {
      case ST_FACE:   eyes.express((FaceId)s.a, s.b); break;
      case ST_SND:    sound.play((SoundId)s.a, (s.b ? s.b : 100) / 100.0f); break;
      case ST_BABBLE: sound.babble(brain.ring[M_HAPPY] / 100.0f, brain.ring[M_ENERGY] / 100.0f); break;
      case ST_GLOW:   glow.flash((GlowC)s.a, s.b); break;
      case ST_MOVE:   if (motion.autoOK) motion.play((MoveId)s.a); break;
      case ST_ARM:    if (motion.autoOK) motion.arm((ArmId)s.a); break;
      case ST_OV:     eyes.overlay((Overlay)s.a, s.b); break;
      case ST_GAZE:   eyes.lookAt(s.a / 100.0f, s.b / 100.0f, 900); break;
      case ST_BLINK:  eyes.blink(); break;
      case ST_BOUNCE: eyes.bounce(1.0f); break;
      default: break;
    }
  }
};

Player player;
