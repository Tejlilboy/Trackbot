// Net.h — WiFi (falls back to its own hotspot), web page, WebSocket, OTA updates, clock.
// The WebSocket callback runs in another task, so it only stores values in `app`;
// the main loop reads them (see handleApp() in Trackbot.ino).
#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>   // library: "ESP Async WebServer" + "Async TCP" (ESP32Async)
#include "config.h"
#include "WebApp.h"
#include "SoundEngine.h"

struct AppState {
  volatile int8_t lx = 0, ly = 0, ry = 0;        // sticks -100..100
  volatile uint32_t lastJoy = 0;
  volatile bool tofView = false;
  volatile int16_t mode = -1, gesture = -1, dbgEvent = -1, dbgPerf = -1, focusMin = -1;   // -1 = nothing pending
  volatile bool party = false, resetMem = false;
  volatile uint8_t bass = 0, mid = 0, treble = 0;
  volatile bool beat = false;
  volatile uint32_t lastMusic = 0;
};
AppState app;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

static void setTune(const char* k, float v) {
  if      (!strcmp(k, "vol"))  tune.volume = v;
  else if (!strcmp(k, "mvol")) tune.musicVol = v;
  else if (!strcmp(k, "glow")) tune.glow = v;
  else if (!strcmp(k, "spd"))  tune.maxSpeed = v;
  else if (!strcmp(k, "kpt"))  tune.kpTurn = v;
  else if (!strcmp(k, "kdt"))  tune.kdTurn = v;
  else if (!strcmp(k, "kpd"))  tune.kpDist = v;
}

static void onWs(AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type != WS_EVT_DATA) return;
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->opcode == WS_TEXT) {
    if (info->index != 0 || len >= 48) return;
    char buf[48]; memcpy(buf, data, len); buf[len] = 0;
    int a, b, c; char key[16]; float v;
    if (buf[0] == 'j' && sscanf(buf, "j,%d,%d,%d", &a, &b, &c) == 3) { app.lx = a; app.ly = b; app.ry = c; app.lastJoy = millis(); }
    else if (buf[0] == 'm' && sscanf(buf, "m,%d", &a) == 1) app.mode = a;
    else if (buf[0] == 'g' && sscanf(buf, "g,%d", &a) == 1) app.gesture = a;
    else if (buf[0] == 'v' && sscanf(buf, "v,%d", &a) == 1) app.tofView = a;
    else if (buf[0] == 'e' && sscanf(buf, "e,%d", &a) == 1) app.dbgEvent = a;
    else if (buf[0] == 'x' && sscanf(buf, "x,%d", &a) == 1) app.dbgPerf = a;
    else if (buf[0] == 'p' && sscanf(buf, "p,%15[^,],%f", key, &v) == 2) setTune(key, v);
    else if (buf[0] == 't' && sscanf(buf, "t,%d", &a) == 1) app.focusMin = a;
    else if (buf[0] == 'k') app.party = true;
    else if (buf[0] == 'r') app.resetMem = true;
    else if (buf[0] == 's') { sound.musicStop(); app.lastMusic = 0; }
  } else {                                         // binary = music; large frames arrive in pieces
    if (info->index == 0 && len >= 5 && data[0] == 0xA1) {
      app.bass = data[1]; app.mid = data[2]; app.treble = data[3]; if (data[4]) app.beat = true;
      app.lastMusic = millis();
      sound.musicPush(data + 5, len - 5);
    } else if (info->index > 0) sound.musicPush(data, len);
  }
}

static void netBegin() {
  WiFi.mode(WIFI_STA); WiFi.setHostname(HOSTNAME); WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(100);
  if (WiFi.status() != WL_CONNECTED) { WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID, AP_PASS); Serial.println("WiFi failed -> hotspot 'Pip' at 192.168.4.1"); }
  else { Serial.print("WiFi OK: http://"); Serial.println(WiFi.localIP()); configTzTime(NTP_TZ, "pool.ntp.org"); }
  MDNS.begin(HOSTNAME);
  ws.onEvent(onWs); server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", INDEX_HTML); });
  server.begin();
  ArduinoOTA.setHostname(HOSTNAME); ArduinoOTA.begin();   // upload over WiFi: port "pip" in Arduino IDE
}
