// Native clock themes require no user-installed package or LittleFS space.
#pragma once
#include "Mode.h"
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
  bool dirty_=true;
  int lastSecond_=-1;
  uint8_t lastFace_=255;
};
extern ClockFaceMode g_clockFaceMode;
