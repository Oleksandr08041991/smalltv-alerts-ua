#include "ClockFaceMode.h"
#include "Gfx.h"
#include "Clock.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <math.h>
#include <time.h>

ClockFaceMode g_clockFaceMode;

namespace {
static const char* const weekdaysUk[]={
  "НЕДІЛЯ","ПОНЕДІЛОК","ВІВТОРОК","СЕРЕДА",
  "ЧЕТВЕР","П'ЯТНИЦЯ","СУБОТА"
};
static void fmtHM(char (&hm)[12],const tm& t){
  snprintf(hm,sizeof(hm),"%02d:%02d",t.tm_hour,t.tm_min);
}
static void fmtDate(char (&date)[24],const tm& t){
  snprintf(date,sizeof(date),"%02d.%02d.%04d",t.tm_mday,t.tm_mon+1,t.tm_year+1900);
}
static void hand(Arduino_GFX* gfx,float radians,int length,uint16_t color) {
  int x=120+(int)(cosf(radians)*length);
  int y=110+(int)(sinf(radians)*length);
  gfx->drawLine(120,110,x,y,color);
}
static void hands(Arduino_GFX* gfx,int hours,int minute,int second,
                  uint16_t primary,uint16_t accent,uint16_t seconds){
  // All hands stop before the outer hour ticks (r >= 79). Erasing the previous
  // hands therefore never damages the dial, so this is a true partial repaint.
  float h=((hours%12)+minute/60.0f)*PI/6.0f-PI/2;
  float m=(minute+second/60.0f)*PI/30.0f-PI/2;
  float sc=second*PI/30.0f-PI/2;
  hand(gfx,h,47,primary);
  hand(gfx,m,70,accent);
  hand(gfx,sc,74,seconds);
  gfx->fillCircle(120,110,4,primary);
}
static void altNeedle(Arduino_GFX* gfx,int minute,uint16_t color){
  // Altimeter-inspired indicator: angular position shows current minutes.
  // This is a clock face, NOT a sensor reading or an actual altitude.
  const float angle=(float)minute*PI/30.0f-PI/2;
  gfx->drawLine(120,107,120+(int)(cosf(angle)*52),107+(int)(sinf(angle)*52),color);
}
static void altTimeWindow(Arduino_GFX* gfx,const tm& t){
  char hm[12];fmtHM(hm,t);
  gfx->fillRoundRect(50,129,140,48,6,gfxTint(0x18E3));
  gfx->drawRoundRect(50,129,140,48,6,C_GRAY);
  gfxDrawCentered(hm,138,4,C_WHITE);
}
static void drawDate(Arduino_GFX* gfx,const tm& t,int y){
  char date[24]; fmtDate(date,t);
  // The inset is intentionally bounded: no change to static borders/lines.
  // Keep every draw inside the 240-pixel panel; the lower dashboard date
  // starts at y=220 and cannot safely request a 23px rectangle from y=218.
  const int top=y-2;
  const int height=(top+23>TFT_HEIGHT)?(TFT_HEIGHT-top):23;
  gfx->fillRect(12,top,216,height,C_BLACK);
  ukrtext::centered(gfx,date,y,2,C_GRAY);
}
} // namespace

// Called only on first paint, mode switch, language/face change or after an
// overlay. Time ticks MUST NOT call this function (it clears the whole panel).
void ClockFaceMode::render(const Settings& s,const tm& t) {
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  const uint8_t face=s.clock.face;
  const bool uk=s.clock.lang=="uk";
  gfx->fillScreen(C_BLACK);
  char hm[12],sec[8];
  fmtHM(hm,t);
  snprintf(sec,sizeof(sec),"%02d",t.tm_sec);
  const uint16_t cyan=gfxTint(0x07DF),orange=gfxTint(0xFD20);

  if(face==1){ // analog with real hour markers
    gfx->drawCircle(120,110,95,C_DGRAY);
    gfx->drawCircle(120,110,93,cyan);
    for(int i=0;i<12;i++){
      float a=(float)i*PI/6.0f-PI/2;
      int x1=120+(int)(cosf(a)*80),y1=110+(int)(sinf(a)*80);
      int x2=120+(int)(cosf(a)*89),y2=110+(int)(sinf(a)*89);
      gfx->drawLine(x1,y1,x2,y2,C_WHITE);
    }
    hands(gfx,t.tm_hour,t.tm_min,t.tm_sec,C_WHITE,cyan,C_RED);
    drawDate(gfx,t,218);
  }else if(face==2){ // information panel
    gfx->fillRoundRect(10,10,220,54,10,gfxTint(0x10A2));
    ukrtext::centered(gfx,uk?"ГОДИННИК":"CLOCK",25,2,cyan);
    gfx->drawRoundRect(9,77,222,91,12,C_DGRAY);
    gfxDrawCentered(hm,92,5,C_WHITE);
    gfxDrawCentered(sec,177,3,orange);
    drawDate(gfx,t,220);
  }else if(face==4){ // aircraft altimeter-inspired clock, no bogus altitude values
    const uint16_t amber=gfxTint(0xFDC0);
    gfx->drawCircle(120,107,99,C_DGRAY);
    gfx->drawCircle(120,107,95,C_WHITE);
    gfx->drawCircle(120,107,89,C_DGRAY);
    for(int i=0;i<60;i++){
      const float a=(float)i*PI/30.0f-PI/2;
      const int outer=88,inner=(i%5==0)?75:82;
      gfx->drawLine(120+(int)(cosf(a)*inner),107+(int)(sinf(a)*inner),
                    120+(int)(cosf(a)*outer),107+(int)(sinf(a)*outer),
                    (i%5==0)?C_WHITE:C_GRAY);
    }
    for(int i=0;i<10;i++){
      char digit[3];snprintf(digit,sizeof(digit),"%d",i);
      const float a=(float)i*PI/5.0f-PI/2;
      gfx->setTextSize(1);
      gfx->setTextColor(amber);
      gfx->setCursor(116+(int)(cosf(a)*63),103+(int)(sinf(a)*63));
      gfx->print(digit);
    }
    ukrtext::centered(gfx,"ALT",15,2,amber);
    altNeedle(gfx,t.tm_min,C_WHITE);
    gfx->fillCircle(120,107,4,amber);
    altTimeWindow(gfx,t);
    ukrtext::centered(gfx,"КИЇВ - ЧАС",205,1,C_GRAY);
  }else if(face==3){ // retro green display
    gfx->drawRect(5,5,230,230,C_GREEN);
    for(int i=0;i<10;i++)gfx->drawFastVLine(13+i*24,13,8,C_DGRAY);
    ukrtext::centered(gfx,uk?"ЧАС УКРАЇНИ":"UKRAINE TIME",36,2,C_GREEN);
    gfxDrawCentered(hm,89,5,C_GREEN);
    drawDate(gfx,t,171);
    ukrtext::centered(gfx,uk?"КИЇВСЬКИЙ ЧАС":"KYIV TIME",207,1,C_GRAY);
  }else{ // minimalist, no fake 1 Hz refresh
    ukrtext::centered(gfx,uk?"ГОДИННИК":"CLOCK",34,2,C_GRAY);
    gfxDrawCentered(hm,82,5,C_WHITE);
    gfx->drawFastHLine(34,146,172,cyan);
    drawDate(gfx,t,164);
    if(uk)ukrtext::centered(gfx,weekdaysUk[t.tm_wday],208,1,cyan);
    else gfxDrawCentered("UKRAINE",208,2,cyan);
  }
}

// Paint only changed parts of the active face. The minimalist and retro faces
// intentionally do not draw every second because they have no seconds display.
void ClockFaceMode::updateTime(const Settings& s,const tm& t,
                               bool minuteChanged,bool dayChanged) {
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  const uint16_t cyan=gfxTint(0x07DF),orange=gfxTint(0xFD20);
  char hm[12],sec[8];fmtHM(hm,t);
  snprintf(sec,sizeof(sec),"%02d",t.tm_sec);
  const uint8_t face=s.clock.face;

  if(face==1){
    if(lastSecond_>=0){
      hands(gfx,lastHour_,lastMinute_,lastSecond_,C_BLACK,C_BLACK,C_BLACK);
    }
    hands(gfx,t.tm_hour,t.tm_min,t.tm_sec,C_WHITE,cyan,C_RED);
    if(dayChanged)drawDate(gfx,t,218);
  }else if(face==2){
    if(minuteChanged){
      gfx->fillRect(29,89,182,46,C_BLACK);
      gfxDrawCentered(hm,92,5,C_WHITE);
    }
    // Smaller isolated seconds rectangle: static surrounding frame remains.
    gfx->fillRect(95,175,50,28,C_BLACK);
    gfxDrawCentered(sec,177,3,orange);
    if(dayChanged)drawDate(gfx,t,220);
  }else if(face==4){
    if(minuteChanged){
      if(lastMinute_>=0) altNeedle(gfx,lastMinute_,C_BLACK);
      altNeedle(gfx,t.tm_min,C_WHITE);
      gfx->fillCircle(120,107,4,gfxTint(0xFDC0));
      altTimeWindow(gfx,t);
    }
  }else if(face==3){
    if(minuteChanged){
      gfx->fillRect(32,87,176,45,C_BLACK);
      gfxDrawCentered(hm,89,5,C_GREEN);
    }
    if(dayChanged)drawDate(gfx,t,171);
  }else{
    if(minuteChanged){
      gfx->fillRect(32,81,176,43,C_BLACK);
      gfxDrawCentered(hm,82,5,C_WHITE);
    }
    if(dayChanged){
      drawDate(gfx,t,164);
      gfx->fillRect(8,206,224,20,C_BLACK);
      if(s.clock.lang=="uk")ukrtext::centered(gfx,weekdaysUk[t.tm_wday],208,1,cyan);
      else gfxDrawCentered("UKRAINE",208,2,cyan);
    }
  }
}
void ClockFaceMode::service(const Settings& s){
  tm t{};
  if(!clockNow(t)){
    if(lastSecond_>=0)dirty_=true; // enter sync screen after a running clock
    if(dirty_){
      Arduino_GFX* gfx=gfxDev();
      if(gfx){
        gfx->fillScreen(C_BLACK);
        if(s.clock.lang=="uk")ukrtext::centered(gfx,"СИНХРОНІЗАЦІЯ",100,2,C_YELLOW);
        else gfxDrawCentered("TIME SYNC",100,2,C_YELLOW);
      }
      dirty_=false;
      lastFace_=255; // restore the complete face once NTP becomes valid
      lastSecond_=-1;
    }
    return;
  }
  const bool uk=s.clock.lang=="uk";
  const bool full=dirty_||s.clock.face!=lastFace_||uk!=lastUkrainian_;
  const bool minuteChanged=t.tm_min!=lastMinute_||t.tm_hour!=lastHour_;
  const bool dayChanged=t.tm_yday!=lastYday_;
  if(full)render(s,t);
  else if(t.tm_sec!=lastSecond_||minuteChanged||dayChanged)
    updateTime(s,t,minuteChanged,dayChanged);
  lastSecond_=t.tm_sec;lastMinute_=t.tm_min;lastHour_=t.tm_hour;
  lastYday_=t.tm_yday;lastFace_=s.clock.face;lastUkrainian_=uk;
  dirty_=false;
}
