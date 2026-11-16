#include "MascotMode.h"
#include "MotivationPhrases.h"
#include "Gfx.h"
#include "Clock.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <time.h>

MascotMode g_mascotMode;
namespace {
constexpr uint32_t kSpeechMs=15000UL;
constexpr uint32_t kRoamMs=60000UL;
static uint16_t navy(){return gfxTint(0x112B);}
static uint16_t sky(){return gfxTint(0x24B8);}
static uint16_t warm(){return gfxTint(0xFDB0);}
static uint16_t backdrop(){return gfxTint(0x0842);}

// Word wrap in UTF-8 without splitting multibyte Ukrainian glyphs.
// Short phrases use 3x (~21px tall) instead of the old 2x bitmap font.
// The longest quotes use 2x so all of the text and author remain visible.
static uint8_t wrapText(const char* phrase,uint8_t fontScale,String lines[5]) {
  for(int i=0;i<5;i++)lines[i]="";
  String word,line;
  uint8_t row=0;
  for(const char* p=phrase;;){
    bool finish=(*p==0);
    if(!finish && *p!=' ') {
      word+=(char)*p++;
      continue;
    }
    String next=line.length()?line+" "+word:word;
    if(ukrtext::textWidth(next.c_str(),fontScale)>210 && line.length()){
      if(row>=5)return 0;
      lines[row++]=line;
      line=word;
    }else line=next;
    word="";
    if(finish)break;
    ++p;
  }
  if(line.length()){
    if(row>=5 || ukrtext::textWidth(line.c_str(),fontScale)>210)return 0;
    lines[row++]=line;
  }
  return row;
}
static void drawQuote(Arduino_GFX* gfx,const char* quote) {
  if(!gfx||!quote)return;
  String lines[5];
  uint8_t scale=3;
  uint8_t n=wrapText(quote,scale,lines);
  if(n==0||n>4) {
    scale=2;
    n=wrapText(quote,scale,lines);
  }
  if(!n)return;
  const int step=(scale==3)?24:19;
  const int first=(scale==3)?39:42;
  for(uint8_t i=0;i<n && i<5;i++)
    ukrtext::centered(gfx,lines[i].c_str(),first+i*step,scale,navy());
}
} // namespace

void MascotMode::begin(const Settings& s){
  (void)s;
  painted_=false;active_=false;selected_=false;dedicated_=false;
  phase_=SPEAK;phaseUntilMs_=0;roamStartMs_=0;
  speechSerial_=0;prevX_=-1;prevY_=-1;
  nextPopupMs_=millis()+120000UL;
  nextAnimationMs_=0;
}
void MascotMode::invalidate(const Settings& s){
  begin(s);
}
bool MascotMode::overlayActive() const{
  return active_ && (int32_t)(untilMs_-millis())>0;
}
void MascotMode::choose(const Settings& s){
  tm now{};
  int month=1,hour=12;
  if(clockNow(now)){month=now.tm_mon+1;hour=now.tm_hour;}
  // Five independent moods selected from time, month and conversation turn.
  // Even repeating the same line can produce a new gesture.
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
  selected_=true;painted_=false;prevX_=-1;nextAnimationMs_=0;
}
void MascotMode::tick(const Settings& s,bool dedicated){
  if(!s.mascot.enabled){
    active_=false;
    dedicated_=false;
    return;
  }
  const uint32_t now=millis();
  if(dedicated){
    active_=false;
    if(!dedicated_){
      dedicated_=true;
      phase_=SPEAK;
      choose(s);
      phaseUntilMs_=now+kSpeechMs;
    }else if((int32_t)(now-phaseUntilMs_)>=0){
      if(phase_==SPEAK){
        phase_=ROAM;
        roamStartMs_=now;
        phaseUntilMs_=now+kRoamMs;
        mood_=(mood_+1u+(speechSerial_%3u))%5u;
        painted_=false;prevX_=-1;nextAnimationMs_=0;
      }else{
        phase_=SPEAK;
        choose(s);
        phaseUntilMs_=now+kSpeechMs;
      }
    }
    return;
  }
  if(dedicated_) {
    dedicated_=false;
    painted_=false;
    prevX_=-1;
  }
  if(active_){
    if(!overlayActive())active_=false;
    return;
  }
  if(!nextPopupMs_)nextPopupMs_=now+120000UL;
  if((int32_t)(now-nextPopupMs_)>=0){
    choose(s);
    active_=true;
    untilMs_=now+kSpeechMs;
    // Clock pop-ups use the separate configured interval. The dedicated
    // mascot screen always uses a precise 60-second roaming phase.
    nextPopupMs_=now+(uint32_t)s.mascot.intervalMin*60000UL;
  }
}
void MascotMode::drawSpeechFrame(const Settings& s){
  (void)s;
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  gfx->fillScreen(backdrop());
  gfx->fillRoundRect(7,7,226,149,14,C_WHITE);
  gfx->drawRoundRect(7,7,226,149,14,sky());
  gfx->drawRoundRect(9,9,222,145,13,gfxTint(0xA6D9));
  gfx->fillTriangle(51,154,80,154,68,172,C_WHITE);
  gfx->drawLine(51,154,68,172,sky());
  gfx->drawLine(68,172,80,154,sky());
  ukrtext::centered(gfx,"ДРУЖНІЙ ПІЛОТ",16,1,sky());
  const motivation::Phrase& p=motivation::get(selectedIndex_);
  drawQuote(gfx,p.text);
  ukrtext::centered(gfx,p.author,141,1,C_GRAY);
  gfx->drawFastHLine(14,229,210,sky());
  painted_=true;nextAnimationMs_=0;
}
void MascotMode::drawSmallCharacter(const Settings& s){
  (void)s;
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  gfx->fillRect(26,173,153,54,backdrop());
  const uint32_t frame=millis()/230UL;
  const int bob=(int)((frame+mood_)%5)-2;
  const int wing=((frame+mood_)%4<2)?6:0;
  const bool blink=((frame+speechSerial_)%24)==0;
  const bool mouthOpen=((frame+mood_)%3)!=0;
  const int cx=94+bob,cy=202+((mood_==2&&(frame&1))?-3:0);
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
  if(mouthOpen)gfx->fillRoundRect(cx-7,cy+2,14,8,4,warm());
  else gfx->drawFastHLine(cx-6,cy+5,12,navy());
  gfx->fillRect(cx-12,cy+16,24,4,warm());
  if(mood_==1||mood_==4){
    if(frame%2==0){
      gfx->fillCircle(cx+52,cy-15,2,warm());
      gfx->drawLine(cx+52,cy-22,cx+52,cy-25,warm());
    }
  }else if(mood_==2)gfx->fillCircle(cx-52,cy-17+(frame%3),2,C_YELLOW);
}
// The larger mascot moves along a rectangle following the SCREEN PERIMETER.
// Position is derived from elapsed time, not repeated accumulated increments,
// so web requests do not freeze the animation timeline.
void MascotMode::drawBigCharacter(int cx,int cy,uint32_t frame){
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  const int flap=((frame+mood_)%4u<2u)?6:-2;
  const bool blink=((frame/2+speechSerial_)%23u)==0u;
  const bool grin=((frame+mood_)%8u)<6u;
  const bool cheer=(mood_==1||mood_==3) && (frame%7u)<3u;
  // Outer wings are 122px wide; central body is 74px vs 46px when speaking.
  gfx->fillRoundRect(cx-62,cy-18-flap,30,13,5,sky());
  gfx->fillRoundRect(cx+32,cy-18+flap,30,13,5,sky());
  gfx->drawFastHLine(cx-68,cy-32-flap,32,C_GRAY);
  gfx->drawFastHLine(cx+36,cy-32+flap,32,C_GRAY);
  gfx->drawFastVLine(cx-51,cy-32-flap,11,C_GRAY);
  gfx->drawFastVLine(cx+51,cy-32+flap,11,C_GRAY);
  gfx->fillRoundRect(cx-37,cy-30,74,67,22,C_WHITE);
  gfx->drawRoundRect(cx-37,cy-30,74,67,22,sky());
  gfx->fillRoundRect(cx-20,cy-32,40,14,6,sky());
  gfx->drawFastHLine(cx-16,cy-24,32,C_WHITE);
  if(blink){
    gfx->drawFastHLine(cx-23,cy-4,13,navy());
    gfx->drawFastHLine(cx+10,cy-4,13,navy());
  }else{
    gfx->fillCircle(cx-17,cy-7,5,navy());
    gfx->fillCircle(cx+17,cy-7,5,navy());
    gfx->fillCircle(cx-18,cy-9,2,C_WHITE);
    gfx->fillCircle(cx+16,cy-9,2,C_WHITE);
  }
  if(grin){
    gfx->drawLine(cx-13,cy+13,cx-7,cy+19,navy());
    gfx->drawFastHLine(cx-7,cy+19,14,navy());
    gfx->drawLine(cx+7,cy+19,cx+13,cy+13,navy());
  }else gfx->fillRoundRect(cx-11,cy+12,22,9,4,warm());
  gfx->fillRoundRect(cx-15,cy+33,30,6,3,warm());
  if(cheer){
    gfx->fillCircle(cx+65,cy-30,3,C_YELLOW);
    gfx->drawLine(cx+65,cy-38,cx+65,cy-42,C_YELLOW);
  }else if(mood_==2&&frame%4u==0u){
    gfx->fillCircle(cx-61,cy+23,3,warm());
  }
}
void MascotMode::drawRoaming(const Settings& s){
  (void)s;
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  if(!painted_){
    gfx->fillScreen(C_BLACK);
    ukrtext::centered(gfx,"АВІАДРУГ",3,1,sky());
    ukrtext::centered(gfx,"НОВА ФРАЗА ЗА ХВИЛИНУ",228,1,C_GRAY);
    painted_=true;prevX_=-1;nextAnimationMs_=0;
  }
  const uint32_t now=millis();
  if(nextAnimationMs_ && (int32_t)(now-nextAnimationMs_)<0)return;
  nextAnimationMs_=now+220UL; // ~4.5 FPS, no framebuffer needed
  const uint32_t frame=(now-roamStartMs_)/220UL;
  const uint32_t dist=((now-roamStartMs_)/160UL*2UL)%404UL;
  int x=74,y=66;
  if(dist<92UL)x+=dist;
  else if(dist<202UL){x=166;y+=(int)(dist-92UL);}
  else if(dist<294UL){x=166-(int)(dist-202UL);y=176;}
  else y=176-(int)(dist-294UL);
  if(mood_==2 && (frame%10u)<3u)y-=2; // occasional happy hop
  if(prevX_>=0){
    // Erase only the former sprite rectangle; never clear the whole screen
    // on frame changes. Top/bottom captions are outside the sprite bounds.
    gfx->fillRect(prevX_-70,prevY_-44,141,87,C_BLACK);
  }
  drawBigCharacter(x,y,frame);
  prevX_=x;prevY_=y;
}
void MascotMode::renderOverlay(const Settings& s){
  if(!selected_)choose(s);
  if(!painted_)drawSpeechFrame(s);
  if(!nextAnimationMs_ || (int32_t)(millis()-nextAnimationMs_)>=0){
    drawSmallCharacter(s);
    nextAnimationMs_=millis()+230UL;
  }
}
void MascotMode::service(const Settings& s){
  if(!s.mascot.enabled){
    if(!painted_){
      Arduino_GFX* gfx=gfxDev();
      if(gfx){
        gfx->fillScreen(C_BLACK);
        ukrtext::centered(gfx,"АВІАДРУГ ВИМКНЕНО",106,2,C_YELLOW);
      }
      painted_=true;
    }
    return;
  }
  tick(s,true);
  if(phase_==ROAM)drawRoaming(s);
  else renderOverlay(s);
}
