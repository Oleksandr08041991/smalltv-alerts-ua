// AlertsMode — background air-alert watcher, mini-map and event overlays.
#pragma once
#include "Mode.h"
#include "config.h"

class AlertsMode : public DisplayMode {
 public:
  const char* id() const override { return "alerts"; }
  uint8_t modeConst() const override { return MODE_ALERTS; }
  void begin(const Settings& s) override;
  void invalidate(const Settings& s) override;
  void wake(const Settings& s) override { (void)s; needRender_ = true; frameReady_ = false; }
  void service(const Settings& s) override;
  void poll(const Settings& s);  // monitor selected region while any clock/theme runs
  bool overlayActive() const { return overlayUntil_ && (int32_t)(overlayUntil_-millis())>0; }
  void renderOverlay(const Settings& s);
  char status() const { return state_; }
  bool fresh(const Settings& s) const;
  uint32_t ageSec() const;
  int lastHttp() const { return lastHttp_; }
  const char* error() const { return error_; }
 private:
  void render(const Settings& s);
  bool fetch(const Settings& s);
  void startOverlay(bool alert);
  char state_='?';   // selected region; ? = unknown
  char oblasts_[28] = {0};   // 27 chars in API official order
  bool failed_=true, hadValid_=false, needRender_=true;
  bool frameReady_=false, paintedFresh_=false;
  bool overlayAlert_=false;
  uint32_t overlayUntil_=0, overlayDrawnAt_=0;
  uint32_t nextPollMs_=0, lastOkMs_=0;
  int lastHttp_=0;
  const char* error_="not configured";
};
extern AlertsMode g_alertsMode;
