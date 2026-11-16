#include "AlertsMode.h"
#include "Gfx.h"
#include "Net.h"
#include "Clock.h"
#include "Platform.h"
#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <memory>

AlertsMode g_alertsMode;

void AlertsMode::begin(const Settings& s) { invalidate(s); }

void AlertsMode::invalidate(const Settings&) {
  nextPollMs_ = 0;
  lastOkMs_ = 0;
  lastHttp_ = 0;
  state_ = '?';
  failed_ = true;
  error_ = "awaiting data";
  needRender_ = true;
}

uint32_t AlertsMode::ageSec() const {
  return lastOkMs_ ? (millis() - lastOkMs_) / 1000UL : 0;
}

bool AlertsMode::fresh(const Settings& s) const {
  // Never report "safe" from the previous fetch after any transport failure.
  const uint32_t maxAge = (uint32_t)s.alerts.pollSec * 3UL;
  return lastOkMs_ && !failed_ && ageSec() <= maxAge;
}

bool AlertsMode::fetch(const Settings& s) {
  if (platformMaxFreeBlock() < 16000) { error_ = "low memory"; return false; }

  // Future hardening: add CA validation before production deployment.
  // The shared platform helper currently creates an insecure TLS client.
  // Never expose token to the browser or compile it into a public binary.
  std::unique_ptr<SecureClient> client(platformMakeSecureClient(4096));
  if (!client) { error_ = "no TLS memory"; return false; }

  HTTPClient http;
  http.useHTTP10(true);  // small JSON string, no chunked transfer needed
  http.setTimeout(s.httpTimeout);
  http.setReuse(false);
  String url = F("https://api.alerts.in.ua/v2/iot/active_air_raid_alerts/");
  url += String(s.alerts.uid);
  url += F(".json");
  if (!http.begin(*client, url)) { error_ = "connection failed"; return false; }

  http.addHeader("Authorization", "Bearer " + s.alerts.token);
  http.addHeader("Accept", "application/json");
  http.setUserAgent("smalltv-alerts-ua/0.1");

  lastHttp_ = http.GET();
  if (lastHttp_ != HTTP_CODE_OK) {
    error_ = lastHttp_ == 401 ? "invalid API token"
           : lastHttp_ == 429 ? "API rate limit" : "HTTP/API error";
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err || !doc.is<const char*>()) { error_ = "invalid JSON"; return false; }
  const char* value = doc.as<const char*>();
  if (!value || !value[0] || value[1] ||
       (value[0] != 'R' && value[0] != 'Y' && value[0] != 'P' && value[0] != 'N')) {
    error_ = "invalid status";
    return false;
  }
  state_ = value[0];
  error_ = "";
  return true;
}

void AlertsMode::poll(const Settings& s) {
  if (!s.alerts.enabled || !s.alerts.token.length()) {
    if (state_ != '?' || !failed_) needRender_ = true;
    failed_ = true;
    error_ = s.alerts.enabled ? "API token required" : "disabled";
    state_ = '?';
    return;
  }
  if (!netConnected()) {
    if (!failed_) needRender_ = true;
    failed_ = true; error_ = "WiFi offline";
    return;
  }
  if (nextPollMs_ && (int32_t)(millis() - nextPollMs_) < 0) return;
  nextPollMs_ = millis() + (uint32_t)s.alerts.pollSec * 1000UL;
  if (fetch(s)) {
    lastOkMs_ = millis() | 1UL;
    failed_ = false;
  } else {
    failed_ = true;
    if (lastHttp_ == 429) nextPollMs_ = millis() + 120000UL;
  }
  needRender_ = true;
}

void AlertsMode::render(const Settings& s) {
  Arduino_GFX* gfx = gfxDev();
  if (!gfx) return;
  gfx->fillScreen(C_BLACK);
  gfxDrawCentered("AIR ALERTS UA", 12, 2, C_WHITE);
  // Built-in GFX 6x8 font is ASCII-only; web form supplies a Latin label.
  const char* label = s.alerts.label.length() ? s.alerts.label.c_str() : "KYIV REGION";
  gfxDrawCentered(label, 39, gfxFitSize(label, 224, 2), C_GRAY);

  uint16_t color = C_YELLOW;
  const char* title = "NO DATA";
  const char* detail = error_;
  if (fresh(s)) {
    switch (state_) {
      case 'R': color = C_RED; title = "AIR ALERT"; detail = "RED LEVEL"; break;
      case 'Y': color = C_YELLOW; title = "WARNING"; detail = "YELLOW LEVEL"; break;
      case 'P': color = C_YELLOW; title = "PARTIAL"; detail = "DISTRICTS"; break;
      case 'N': color = C_GREEN; title = "NO ALERT"; detail = "LAST CHECK OK"; break;
      default: break;
    }
  }
  gfx->fillRoundRect(13, 76, 214, 91, 12, color);
  gfx->fillRoundRect(17, 80, 206, 83, 10, C_BLACK);
  gfxDrawCentered(title, 98, gfxFitSize(title, 195, 3), color);
  gfxDrawCentered(detail, 141, 1, color);

  struct tm t;
  if (clockNow(t)) {
    char hm[8];
    snprintf(hm, sizeof(hm), "%02d:%02d", t.tm_hour, t.tm_min);
    gfxDrawCentered(hm, 181, 3, C_WHITE);
  } else {
    gfxDrawCentered("TIME SYNC...", 186, 1, C_GRAY);
  }
  if (fresh(s)) {
    char age[32];
    snprintf(age, sizeof(age), "UPDATED %lus AGO", (unsigned long)ageSec());
    gfxDrawCentered(age, 219, 1, C_GRAY);
  } else {
    gfxDrawCentered("VERIFY CONNECTION", 219, 1, C_YELLOW);
  }
}

void AlertsMode::service(const Settings& s) {
  if (needRender_ || millis() - lastRenderMs_ > 15000UL) {
    render(s);
    needRender_ = false;
    lastRenderMs_ = millis();
  }
}
