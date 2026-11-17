// Animated aviation friend: 60 s speech, 30 s roaming Tamagotchi.
// Rule-driven mood and gestures, no online AI model or extra framebuffer.
#pragma once
#include "Mode.h"
class MascotMode : public DisplayMode {
public:
  const char* id() const override { return "mascot"; }
  uint8_t modeConst() const override { return MODE_MASCOT; }
  void begin(const Settings& s) override;
  void invalidate(const Settings& s) override;
  void wake(const Settings& s) override {
    (void)s;
    painted_=false;
    prevX_=-1;
    nextAnimationMs_=0;
    carouselActive_=false;
  }
  void service(const Settings& s) override;
  void tick(const Settings& s,bool dedicated);
  bool overlayActive() const;
  void renderOverlay(const Settings& s);
private:
  enum Phase : uint8_t { SPEAK=0, ROAM=1 };
  void choose(const Settings& s);
  void drawSpeechFrame(const Settings& s);
  void drawSmallCharacter(const Settings& s);
  void drawRoaming(const Settings& s);
  void drawBigCharacter(int cx,int cy,uint32_t frame);
  bool painted_=false, active_=false, selected_=false, dedicated_=false;
  bool carouselActive_=false;
  uint32_t carouselQuoteUntilMs_=0;
  uint32_t prevFrame_=0,prevSmallFrame_=0;
  uint8_t prevMood_=0,prevSmallMood_=0;
  int16_t prevSmallX_=-1,prevSmallY_=-1;
  Phase phase_=SPEAK;
  uint32_t phaseUntilMs_=0, roamStartMs_=0;
  uint32_t nextPopupMs_=0, untilMs_=0, nextAnimationMs_=0;
  uint16_t speechSerial_=0;
  uint8_t motionVariant_=0;  // 20 distinct flight motions after phrases
  uint8_t selectedIndex_=0,mood_=0;
  int16_t prevX_=-1,prevY_=-1;
};
extern MascotMode g_mascotMode;
