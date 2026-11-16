#include "ClockFaceMode.h"
#include "Gfx.h"
#include "Clock.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <math.h>
#include <time.h>

ClockFaceMode g_clockFaceMode;
static const char* const weekdaysUk[]={
  "НЕДІЛЯ","ПОНЕДІЛОК","ВІВТОРОК","СЕРЕДА",
  "ЧЕТВЕР","П'ЯТНИЦЯ","СУБОТА"
};
void ClockFaceMode::render(const Settings& s,const tm& t) {
  Arduino_GFX* gfx=gfxDev(); if(!gfx)return;
  uint8_t face=s.clock.face;
  bool uk=(s.clock.lang=="uk");
  gfx->fillScreen(C_BLACK);
  char hm[12],sec[8],date[24];
  snprintf(hm,sizeof(hm),"%02d:%02d",t.tm_hour,t.tm_min);
  snprintf(sec,sizeof(sec),"%02d",t.tm_sec);
  snprintf(date,sizeof(date),"%02d.%02d.%04d",t.tm_mday,t.tm_mon+1,t.tm_year+1900);
  uint16_t cyan=gfxTint(0x07DF), orange=gfxTint(0xFD20);

  if(face==1) { // analog
    gfx->drawCircle(120,110,95,C_DGRAY);
    gfx->drawCircle(120,110,93,cyan);
    for(int i=0;i<12;i++){
      float a=(float)i*PI/6.0f-PI/2;
      int x1=120+(int)(cosf(a)*79),y1=110+(int)(sinf(a)*79);
      int x2=120+(int)(cosf(a)*89),y2=110+(int)(sinf(a)*89);
      gfx->drawLine(x1,y1,x2,y2,C_WHITE);
    }
    float h=((t.tm_hour%12)+t.tm_min/60.0f)*PI/6.0f-PI/2;
    float m=(t.tm_min+t.tm_sec/60.0f)*PI/30.0f-PI/2;
    float sc=t.tm_sec*PI/30.0f-PI/2;
    gfx->drawLine(120,110,120+(int)(cosf(h)*47),110+(int)(sinf(h)*47),C_WHITE);
    gfx->drawLine(120,110,120+(int)(cosf(m)*73),110+(int)(sinf(m)*73),cyan);
    gfx->drawLine(120,110,120+(int)(cosf(sc)*85),110+(int)(sinf(sc)*85),C_RED);
    gfx->fillCircle(120,110,4,C_WHITE);
    ukrtext::centered(gfx,date,218,2,C_GRAY);
  } else if(face==2) { // dashboard
    gfx->fillRoundRect(10,10,220,54,10,gfxTint(0x10A2));
    ukrtext::centered(gfx,uk?"ГОДИННИК":"CLOCK",25,2,cyan);
    gfx->drawRoundRect(9,77,222,91,12,C_DGRAY);
    gfxDrawCentered(hm,92,5,C_WHITE);
    gfxDrawCentered(sec,177,3,orange);
    ukrtext::centered(gfx,date,220,2,C_GRAY);
  } else if(face==3) { // retro
    gfx->drawRect(5,5,230,230,C_GREEN);
    for(int i=0;i<10;i++)gfx->drawFastVLine(13+i*24,13,8,C_DGRAY);
    ukrtext::centered(gfx,uk?"ЧАС УКРАЇНИ":"UKRAINE TIME",36,2,C_GREEN);
    gfxDrawCentered(hm,89,5,C_GREEN);
    ukrtext::centered(gfx,date,171,2,C_WHITE);
    ukrtext::centered(gfx,uk?"КИЇВСЬКИЙ ЧАС":"KYIV TIME",207,1,C_GRAY);
  } else { // minimalist digital
    ukrtext::centered(gfx,uk?"ГОДИННИК":"CLOCK",34,2,C_GRAY);
    gfxDrawCentered(hm,82,5,C_WHITE);
    gfx->drawFastHLine(34,146,172,cyan);
    ukrtext::centered(gfx,date,164,2,C_WHITE);
    if(uk)ukrtext::centered(gfx,weekdaysUk[t.tm_wday],208,1,cyan);
    else gfxDrawCentered("UKRAINE",208,2,cyan);
  }
}
void ClockFaceMode::service(const Settings& s){
  tm t{};
  if(!clockNow(t)) {
    if(dirty_){
      Arduino_GFX* gfx=gfxDev();
      if(gfx){
        gfx->fillScreen(C_BLACK);
        if(s.clock.lang=="uk") ukrtext::centered(gfx,"СИНХРОНІЗАЦІЯ",100,2,C_YELLOW);
        else gfxDrawCentered("TIME SYNC",100,2,C_YELLOW);
      }
      dirty_=false;
    }
    return;
  }
  if(dirty_ || t.tm_sec!=lastSecond_ || s.clock.face!=lastFace_) {
    lastSecond_=t.tm_sec;lastFace_=s.clock.face;dirty_=false;
    render(s,t);
  }
}
