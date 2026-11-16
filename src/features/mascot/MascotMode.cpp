#include "MascotMode.h"
#include "MotivationPhrases.h"
#include "Gfx.h"
#include "Clock.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <time.h>

MascotMode g_mascotMode;
static uint16_t navy(){return gfxTint(0x112B);}
static uint16_t sky(){return gfxTint(0x24B8);}
static uint16_t warm(){return gfxTint(0xFDB0);}
void MascotMode::begin(const Settings& s){
  (void)s;
  painted_=false;
  active_=false;
  selected_=false;
  speechSerial_=0;
  nextPopupMs_=millis()+120000UL;
  nextSentenceMs_=0;
  nextAnimationMs_=0;
}
void MascotMode::invalidate(const Settings& s) {
  (void)s;
  painted_=false;
  active_=false;
  selected_=false;
  nextPopupMs_=millis()+120000UL;
}
bool MascotMode::overlayActive() const {
  return active_ && (int32_t)(untilMs_-millis())>0;
}
void MascotMode::choose(const Settings& s){
  tm now{};
  int month=1,hour=12;
  if(clockNow(now)){month=now.tm_mon+1;hour=now.tm_hour;}
  // Rule-based "micro AI": different personality and gesture for the same
  // quotation depending on month, time of day and conversational turn.
  uint32_t seed=(uint32_t)month*61UL+(uint32_t)hour*23UL+
                (uint32_t)++speechSerial_*37UL+(millis()/1000UL)%31UL;
  uint8_t desired=s.mascot.focus;
  uint8_t start=(uint8_t)(seed%motivation::count);
  uint8_t index=start;
  for(int i=0;i<motivation::count;i++){
    index=(uint8_t)((start+i*37)%motivation::count);
    if(desired==0||(uint8_t)motivation::get(index).business==(desired==1?1:0))
      break;
  }
  selectedIndex_=index;
  mood_=(uint8_t)((seed/7+hour+speechSerial_)%5);
  selected_=true;painted_=false;
}
void MascotMode::tick(const Settings& s,bool dedicated){
  if(!s.mascot.enabled){
    active_=false;
    return;
  }
  const uint32_t now=millis();
  if(dedicated){
    if(!selected_||(int32_t)(now-nextSentenceMs_)>=0){
      choose(s);
      nextSentenceMs_=now+15000UL;
    }
    active_=false;
    return;
  }
  if(active_){
    if(!overlayActive())active_=false;
    return;
  }
  if(!nextPopupMs_)nextPopupMs_=now+120000UL;
  if((int32_t)(now-nextPopupMs_)>=0){
    choose(s);
    active_=true;
    untilMs_=now+15000UL;
    nextPopupMs_=now+(uint32_t)s.mascot.intervalMin*60000UL;
  }
}
static void printQuote(Arduino_GFX* gfx,const char* quote) {
  if(!gfx||!quote)return;
  const int maxW=204;
  String word,line;
  int row=0;
  const char* p=quote;
  while(true){
    bool end=(*p==0);
    if(!end&&*p!=' '){
      // Keep the UTF-8 bytes intact, including two-byte Ukrainian glyphs.
      word+=(char)*p++;
      continue;
    }
    String candidate=line.length()?line+" "+word:word;
    if(ukrtext::textWidth(candidate.c_str(),2)>maxW&&line.length()){
      ukrtext::centered(gfx,line.c_str(),37+row*24,2,navy());
      row++;
      line=word;
    }else line=candidate;
    word="";
    if(end)break;
    p++;
  }
  if(line.length()&&row<4)ukrtext::centered(gfx,line.c_str(),37+row*24,2,navy());
}
void MascotMode::drawStatic(const Settings& s){
  (void)s;
  Arduino_GFX* gfx=gfxDev();
  if(!gfx)return;
  // Static white bubble + blue outline, with tail toward mascot at bottom.
  // The text region is much larger than the character (user requirement).
  gfx->fillScreen(gfxTint(0x0842));
  gfx->fillRoundRect(7,7,226,149,14,C_WHITE);
  gfx->drawRoundRect(7,7,226,149,14,sky());
  gfx->drawRoundRect(9,9,222,145,13,gfxTint(0xA6D9));
  gfx->fillTriangle(51,154,80,154,68,172,C_WHITE);
  gfx->drawLine(51,154,68,172,sky());
  gfx->drawLine(68,172,80,154,sky());
  ukrtext::centered(gfx,"ДРУЖНІЙ ПІЛОТ",16,1,sky());
  const motivation::Phrase& p=motivation::get(selectedIndex_);
  printQuote(gfx,p.text);
  ukrtext::centered(gfx,p.author,136,1,C_GRAY);
  gfx->drawFastHLine(13,158,214,gfxTint(0x3192));
  // Minimal ground/horizon artwork leaves the comic quote unobstructed.
  gfx->drawFastHLine(14,229,210,sky());
  painted_=true;
  nextAnimationMs_=0;
}
void MascotMode::drawCharacter(const Settings& s){
  (void)s;
  Arduino_GFX* gfx=gfxDev(); if(!gfx)return;
  // Only this small lower area is cleared every ~250ms.
  gfx->fillRect(26,173,153,54,gfxTint(0x0842));
  const uint32_t frame=millis()/230UL;
  const int bob=(int)((frame+mood_)%5)-2;
  const int wing=((frame+mood_)%4<2)?6:0;
  const bool blink=((frame+speechSerial_)%24)==0;
  const bool mouthOpen=((frame+mood_)%3)!=0;
  const int cx=94+bob,cy=202+((mood_==2&&(frame&1))?-3:0);
  // Compact, cheerful cartoon aircraft/drone with wings and tiny rotors.
  gfx->fillRoundRect(cx-36,cy-13-wing,20,8,3,sky());
  gfx->fillRoundRect(cx+16,cy-13+wing,20,8,3,sky());
  gfx->drawFastHLine(cx-47,cy-21-wing,24,C_GRAY);
  gfx->drawFastHLine(cx+23,cy-21+wing,24,C_GRAY);
  gfx->drawFastVLine(cx-35,cy-21-wing,8,C_GRAY);
  gfx->drawFastVLine(cx+35,cy-21+wing,8,C_GRAY);
  gfx->fillRoundRect(cx-23,cy-23,46,38,15,C_WHITE);
  gfx->drawRoundRect(cx-23,cy-23,46,38,15,sky());
  gfx->fillRoundRect(cx-12,cy-24,24,9,4,sky());
  if(blink){
    gfx->drawFastHLine(cx-13,cy-6,8,navy());
    gfx->drawFastHLine(cx+5,cy-6,8,navy());
  }else{
    gfx->fillCircle(cx-9,cy-7,3,navy());
    gfx->fillCircle(cx+9,cy-7,3,navy());
    gfx->drawPixel(cx-10,cy-8,C_WHITE);
    gfx->drawPixel(cx+8,cy-8,C_WHITE);
  }
  // The animated mouth opens/closes in sync with the comic speech beat.
  if(mouthOpen)gfx->fillRoundRect(cx-7,cy+2,14,8,4,warm());
  else gfx->drawFastHLine(cx-6,cy+5,12,navy());
  gfx->fillRect(cx-12,cy+16,24,4,warm());
  // Mood-dependent cheer/salute particles live outside the speech bubble.
  if(mood_==1||mood_==4){
    if(frame%2==0){
      gfx->fillCircle(cx+52,cy-15,2,warm());
      gfx->drawLine(cx+52,cy-22,cx+52,cy-25,warm());
    }
  }else if(mood_==2){
    gfx->fillCircle(cx-52,cy-17+(frame%3),2,C_YELLOW);
  }
}
void MascotMode::renderOverlay(const Settings& s){
  if(!selected_)choose(s);
  if(!painted_)drawStatic(s);
  if((int32_t)(millis()-nextAnimationMs_)>=0){
    drawCharacter(s);
    nextAnimationMs_=millis()+230UL;
  }
}
void MascotMode::service(const Settings& s){
  tick(s,true);
  renderOverlay(s);
}
