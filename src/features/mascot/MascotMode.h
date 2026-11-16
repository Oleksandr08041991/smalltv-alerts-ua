// Animated aviation mascot. Local rule-based behaviour, no LLM/API required.
#pragma once
#include "Mode.h"
class MascotMode : public DisplayMode {
public:
  const char* id() const override { return "mascot"; }
  uint8_t modeConst() const override { return MODE_MASCOT; }
  void begin(const Settings& s) override;
  void invalidate(const Settings& s) override;
  void wake(const Settings& s) override { (void)s; painted_=false; }
  void service(const Settings& s) override;
  void tick(const Settings& s, bool dedicated);
  bool overlayActive() const;
  void renderOverlay(const Settings& s);
private:
  void choose(const Settings& s);
  void drawStatic(const Settings& s);
  void drawCharacter(const Settings& s);
  bool painted_=false, active_=false, selected_=false;
  uint32_t nextPopupMs_=0, untilMs_=0, nextSentenceMs_=0, nextAnimationMs_=0;
  uint16_t speechSerial_=0;
  uint8_t selectedIndex_=0, mood_=0;
};
extern MascotMode g_mascotMode;
