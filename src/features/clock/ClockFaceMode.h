// ClockFaceMode.h — partial repaint clock (no full-screen 1 Hz blinking).
#pragma once
#include "Mode.h"
class Arduino_GFX;
class ClockFaceMode : public DisplayMode {
 public:
  const char* id() const override { return "clock"; }
  uint8_t modeConst() const override { return MODE_CLOCK; }
  void begin(const Settings& s) override { (void)s; dirty_=true; }
  void invalidate(const Settings& s) override { (void)s; dirty_=true; }
  void wake(const Settings& s) override { (void)s; dirty_=true; }
  void service(const Settings& s) override;
 private:
  void render(const Settings& s, const struct tm& t);
  void updateTime(const Settings& s, const struct tm& t, bool minuteChanged, bool dayChanged);
  bool dirty_=true;
  int lastSecond_=-1, lastMinute_=-1, lastHour_=-1, lastYday_=-1;
  uint8_t lastFace_=255;
  bool lastUkrainian_=false;
};
extern ClockFaceMode g_clockFaceMode;
