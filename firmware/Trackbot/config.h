// config.h — EVERY pin, switch and tunable for Pip lives here.
// Values marked TUNE are guesses until tested on the real robot.
#pragma once
#include <Arduino.h>

// ---------- Feature switches (set 0 to bring Pip up one part at a time) ----------
#define USE_OLED    1
#define USE_AUDIO   1
#define USE_LED     1
#define USE_MOTORS  1
#define USE_SERVO   1
#define USE_IMU     1
#define USE_TOUCH   1
#define USE_TOF     1   // needs "SparkFun VL53L5CX Arduino Library"
#define USE_WIFI    1
#define AUTO_MOTION 1   // 0 = Pip never drives on his own (safe for desk testing)

// ---------- Pins (XIAO ESP32-S3, GPIO numbers) ----------
#define PIN_SPK      1   // D0  -> PAM8302 A+
#define PIN_TOUCH    2   // D1  copper-tape head pad
#define PIN_SPARE    3   // D2  spare (mic later)
#define PIN_LED      4   // D3  WS2812B data (330R in series)
#define PIN_SDA      5   // D4
#define PIN_SCL      6   // D5
#define PIN_MA1     43   // D6  DRV8833 AIN1
#define PIN_MA2     44   // D7  DRV8833 AIN2
#define PIN_MB1      7   // D8  DRV8833 BIN1
#define PIN_MB2      8   // D9  DRV8833 BIN2
#define PIN_SERVO    9   // D10 forklift MG90S

// ---------- I2C ----------
#define I2C_HZ     400000
#define OLED_ADDR  0x3C
#define IMU_ADDR   0x68

// ---------- WiFi ----------
// Real credentials go in secrets.h (git-ignored). Copy secrets.example.h -> secrets.h.
#if __has_include("secrets.h")
  #include "secrets.h"
#endif
#ifndef WIFI_SSID
  #define WIFI_SSID  "YOUR_WIFI"
  #define WIFI_PASS  "YOUR_PASSWORD"
#endif
#ifndef AP_PASS
  #define AP_PASS    "pip12345"
#endif
#define AP_SSID    "Pip"            // fallback hotspot if WiFi fails (open http://192.168.4.1)
#define HOSTNAME   "pip"            // http://pip.local
#define NTP_TZ     "EST5EDT,M3.2.0,M11.1.0"

// ---------- Motors (TUNE) ----------
#define MOTOR_PWM_HZ   20000
#define MOTOR_A_DIR    1      // flip to -1 if left track runs backwards
#define MOTOR_B_DIR   -1      // mirrored motor
#define MOTOR_SWAP     0      // 1 if A is actually the right track
#define MOTOR_MIN      0.22f  // deadband: lowest power that actually moves
#define MOTOR_RAMP     4.0f   // power units per second (smooth starts)
#define MANUAL_HOLD_MS 400    // joystick counts as "in control" this long after last packet
#define MANUAL_PHEW_MS 5000   // idle sticks this long -> "phew", back to autonomous

// ---------- Forklift servo (TUNE: never drive into hard stops) ----------
#define ARM_DOWN_DEG   20
#define ARM_UP_DEG    110
#define ARM_RATE       0.9f   // full-stick travel per second (0..1 range)
#define ARM_EASE       9.0f

// ---------- Eyes ----------
#define EYE_GAP        23     // px from screen center to each eye center
#define EYE_EASE       14.0f  // how fast the face morphs
#define GAZE_EASE      16.0f
#define BLINK_MIN_MS   1800
#define BLINK_MAX_MS   6000
#define GRAVITY_GAIN   1.6f   // gravity eyes strength
#define GRAVITY_SIGN_X 1      // flip if pupils slide uphill
#define GRAVITY_SIGN_Y 1

// ---------- Audio ----------
#define AUDIO_RATE   16000
#define AUDIO_BLOCK  256
#define MUSIC_BUF    24000    // 1.5 s of streamed music
#define MUSIC_PREBUF 4000

// ---------- Glow ----------
#define GLOW_EASE    4.0f

// ---------- Touch (TUNE) ----------
#define TOUCH_RATIO   1.15f   // touched when reading > baseline * this (S3 values go UP)
#define TAP_MS        300
#define PET_START_MS  450
#define PET_TICK_MS   1200
#define OVERPET_MS    11000

// ---------- IMU (TUNE). Map chip axes to robot axes for how the board is mounted ----------
#define IMU_FWD(ax, ay, az)  (ax)
#define IMU_LEFT(ax, ay, az) (ay)
#define IMU_UP(ax, ay, az)   (az)
#define SHAKE_G        0.7f   // spike size (g) that counts toward a shake
#define SHAKE_COUNT    4
#define SHAKE_WINDOW   800
#define HELD_DPS       25.0f  // rotation that means "someone is holding me"
#define SET_DPS        6.0f
#define KNOCK_G        0.35f
#define BUMP_G         0.6f

// ---------- ToF (TUNE) ----------
#define TOF_HZ          15
#define TOF_FLIP_X      0     // fix after the wave-your-hand test
#define TOF_FLIP_Y      0
#define TOF_TRANSPOSE   0
#define TOF_FLOOR_ROWS  2     // bottom rows watch the table
#define HAND_MAX_MM     400
#define HAND_MIN_ZONES  3
#define HAND_FAST_MMS   450   // faster approach than this = startle
#define EDGE_JUMP_MM    80
#define COVER_MM        45    // peekaboo: this close across most of the grid
#define FOLLOW_DIST_MM  150

// ---------- Brain ----------
#define RING_RATE      0.035f // per second: ring drifts back to core
#define CORE_RATE      0.004f // per second: core follows ring (his "day")
#define BORED_RATE     0.4f
#define ENERGY_DRAIN   0.06f
#define ENERGY_SLEEP   0.25f  // a nap lasts ~5 min
#define IDLE_MIN_MS    5000   // gap between idle behaviors
#define IDLE_MAX_MS    14000

// ---------- Wander (TUNE) ----------
#define WANDER_SPEED     0.45f  // forward power while strolling
#define WANDER_TURN      0.45f  // turn power while scanning
#define WANDER_TURN_DPS  70.0f  // deg/s at that power; only used if the IMU is missing
#define WANDER_OBST_MM   180    // stop when the middle of the view is closer than this

// ---------- Misc personality ----------
#define STUBBORN_ANGER   50     // angrier than this -> he may ignore the joystick for a second
#define MIRROR_MAX_MM    350    // arm mirrors your hand when it's closer than this
#define SAVE_EVERY_MS    300000 // save his memory to flash every 5 min

// ---------- Live-tunable from the web app ----------
struct Tune {
  float volume   = 0.6f;   // master
  float musicVol = 0.8f;
  float glow     = 0.45f;  // LED brightness 0..1
  float maxSpeed = 1.0f;   // joystick speed cap
  float kpTurn   = 0.22f;  // follow: turn per column of error
  float kdTurn   = 0.10f;
  float kpDist   = 0.004f; // follow: power per mm of error
};
Tune tune;

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float frand(float lo, float hi) { return lo + (hi - lo) * (random(10000) / 10000.0f); }
