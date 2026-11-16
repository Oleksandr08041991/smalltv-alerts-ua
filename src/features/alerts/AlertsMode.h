// AlertsMode.h — independent alerts.in.ua display mode (Ukraine, ESP8266/ESP32).
#pragma once
#include "Mode.h"
#include "config.h"

class AlertsMode : public DisplayMode {
 public:
  const char* id() const override { return "alerts"; }
  uint8_t modeConst() const override { return MODE_ALERTS; }
  void begin(const Settings& s) override;
  void invalidate(const Settings& s) override;
  void wake(const Settings& s) override { needRender_ = true; }
  void service(const Settings& s) override;
  void poll(const Settings& s);   // may run while another screen is visible
  char status() const { return state_; }
  bool fresh(const Settings& s) const;
  uint32_t ageSec() const;
  int lastHttp() const { return lastHttp_; }
  const char* error() const { return error_; }
 private:
  void render(const Settings& s);
  bool fetch(const Settings& s);
  char state_ = '?';   // R / Y / P / N on success; ? = never succeeded
  bool failed_ = true;
  bool needRender_ = true;
  uint32_t nextPollMs_ = 0;
  uint32_t lastOkMs_ = 0;
  uint32_t lastRenderMs_ = 0;
  int lastHttp_ = 0;
  const char* error_ = "not configured";
};

extern AlertsMode g_alertsMode;
