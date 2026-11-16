#include "MascotMode.h"
#include "MotivationPhrases.h"
#include "Gfx.h"
#include "Clock.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <time.h>

MascotMode g_mascotMode;
namespace {
constexpr uint32_t kSpeechMs=60000UL;
constexpr uint32_t kPopupMs=15000UL;
constexpr uint32_t kCarouselQuoteMs=12000UL;
constexpr uint32_t kRoamMs=30000UL;
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

// The mascot is composited by pixels, without a full-frame buffer.
// Old and new poses are analytically sampled, and only DIFFERENT color runs
// are sent over SPI. This eliminates the black erase rectangle between frames
// on the slow ST7789 display and needs under 100 bytes of scratch memory.
static inline bool disc(int x,int y,int cx,int cy,int radius) {
  const int dx=x-cx,dy=y-cy;return dx*dx+dy*dy<=radius*radius;
}
static inline bool ellipse(int x,int y,int cx,int cy,int rx,int ry){
  const int dx=x-cx,dy=y-cy;
  return (int32_t)dx*dx*ry*ry+(int32_t)dy*dy*rx*rx <= (int32_t)rx*rx*ry*ry;
}
static inline bool rounded(int x,int y,int left,int top,int width,int height,int r){
  if(x<left||x>=left+width||y<top||y>=top+height)return false;
  const int px=max(left+r,min(x,left+width-r-1));
  const int py=max(top+r,min(y,top+height-r-1));
  const int dx=x-px,dy=y-py;
  return dx*dx+dy*dy<=r*r;
}
// 0=transparent, 1=body border, 2=blue wing, 3=white shell, 4=navy,
// 5=gold, 6=grey, 7=cyan highlight, 8=pale-blue face, 9=coral cheek,
// 10=yellow star, 11=teal. Each frame has two independent rotor/wing poses.
static uint8_t mascotPixel(int x,int y,uint32_t frame,uint8_t mood,uint16_t speech){
  if(x<-72||x>72||y<-44||y>45)return 0;
  const int flap=((frame+mood)%6u<3u)?3:-3;
  const bool blink=((frame/2u+speech)%29u)==0u;
  const bool grin=((frame+mood)%8u)<6u;
  uint8_t c=0;
  // Twin little sparkles appear on the happy moods, behind the silhouette.
  if((mood==1||mood==3)&&frame%5u<3u){
    if(disc(x,y,62,-35,3)||disc(x,y,-61,31,2))c=10;
  }
  // Soft aviation exhaust / attitude fins near the rear.
  if(rounded(x,y,-54,10,25,13,5)||rounded(x,y,29,10,25,13,5))c=1;
  if(rounded(x,y,-50,12,20,8,4)||rounded(x,y,30,12,20,8,4))c=7;

  // Twin winglets. Dark aerodynamic outline + bright upper wing surfaces.
  if(rounded(x,y,-68,-22-flap,40,17,7)||
     rounded(x,y,28,-22+flap,40,17,7))c=1;
  if(rounded(x,y,-65,-20-flap,37,12,6)||
     rounded(x,y,28,-20+flap,37,12,6))c=2;
  if(rounded(x,y,-60,-18-flap,19,4,2)||
     rounded(x,y,41,-18+flap,19,4,2))c=7;
  // Propeller struts and two-blade rotor discs.
  if((abs(x+52)<=2 && y>=-37-flap && y<=-25-flap)||
     (abs(x-52)<=2 && y>=-37+flap && y<=-25+flap))c=6;
  if((y>=-39-flap&&y<=-37-flap&&abs(x+52)<=17)||
     (y>=-39+flap&&y<=-37+flap&&abs(x-52)<=17))c=7;
  if((y==-38-flap && abs(x+52)<=8)||
     (y==-38+flap && abs(x-52)<=8))c=3;

  // Rounded fuselage has a navy rim, a sky-blue frame and pearl-white face.
  if(rounded(x,y,-40,-33,80,77,24))c=1;
  if(rounded(x,y,-38,-31,76,73,22))c=2;
  if(rounded(x,y,-35,-27,70,65,20))c=3;
  if(ellipse(x,y,0,-9,31,20))c=8;
  if(ellipse(x,y,-19,-17,8,5)||ellipse(x,y,19,-17,8,5))c=3;
  // Bright aircraft visor, central bolt and eyebrow accents.
  if(rounded(x,y,-23,-34,46,11,5))c=1;
  if(rounded(x,y,-21,-33,42,8,4))c=7;
  if(rounded(x,y,-13,-30,26,3,1))c=3;
  if(disc(x,y,0,-31,3))c=5;
  if(y>=-14&&y<=-12&&((x>=-26&&x<=-11)||(x>=11&&x<=26)))c=2;

  if(blink){
    if(y>=-4&&y<=-2&&((x>=-23&&x<=-12)||(x>=12&&x<=23)))c=4;
  }else{
    if(ellipse(x,y,-17,-4,6,8)||ellipse(x,y,17,-4,6,8))c=4;
    if(disc(x,y,-19,-7,2)||disc(x,y,15,-7,2))c=3;
  }
  if(ellipse(x,y,-27,12,5,3)||ellipse(x,y,27,12,5,3))c=9;
  // Mouth reacts even when walking; larger opening on an excited mood.
  if(grin){
    if(y>=12&&y<=19){
      const int dy=y-12;
      if(abs(x)<=10&&y>=15+abs(x)/5)c=4;
      else if(abs(x)>=9&&abs(x)<=13&&dy>=abs(x)-9&&dy<=7)c=4;
    }
  }else if(ellipse(x,y,0,17,12,7)){
    c=4;
    if(y>=20&&ellipse(x,y,0,22,8,3))c=9;
  }
  // Belly insignia, aviation badge and tiny boots.
  if(rounded(x,y,-15,32,30,7,3))c=5;
  if(rounded(x,y,-21,36,14,6,2)||rounded(x,y,7,36,14,6,2))c=1;
  if(rounded(x,y,-19,37,10,4,2)||rounded(x,y,9,37,10,4,2))c=7;
  return c;
}
static uint16_t mascotColor(uint8_t c,uint16_t background) {
  switch(c){
    case 0:return background;
    case 1:return gfxTint(0x0948); // navy outline
    case 2:return gfxTint(0x24B8); // aviation blue
    case 3:return C_WHITE;
    case 4:return gfxTint(0x112B);
    case 5:return gfxTint(0xFDB0);
    case 6:return C_GRAY;
    case 7:return gfxTint(0x4E7F); // cyan highlight
    case 8:return gfxTint(0xDEFF); // cockpit glass
    case 9:return gfxTint(0xF2CC); // cheeks/tongue
    case 10:return C_YELLOW;
    default:return gfxTint(0x0555);
  }
}
static inline uint8_t spriteAt(int screenX,int screenY,int cx,int cy,
                               uint32_t frame,uint8_t mood,uint16_t serial,bool mini) {
  if(cx<0)return 0;
  const int dx=screenX-cx,dy=screenY-cy;
  // Half-to-two-thirds size on the talking card, full size when roaming.
  return mini?mascotPixel(dx*3/2,dy*3/2,frame,mood,serial)
             :mascotPixel(dx,dy,frame,mood,serial);
}
// Batch consecutive differing pixels into horizontal monochrome palette runs.
// No framebuffer, fillRect erase, screen flashing or 16-bit canvas allocation.
static void drawSpriteDelta(Arduino_GFX* gfx,
                            int x,int y,uint32_t frame,uint8_t mood,
                            int oldX,int oldY,uint32_t oldFrame,uint8_t oldMood,
                            uint16_t serial,bool mini,uint16_t bg) {
  if(!gfx)return;
  const int rx=mini?49:72,ry=mini?30:45;
  const int minX=max(0,min(x-rx,oldX>=0?oldX-rx:x-rx));
  const int maxX=min(239,max(x+rx,oldX>=0?oldX+rx:x+rx));
  const int minY=max(mini?173:23,min(y-ry,oldX>=0?oldY-ry:y-ry));
  const int maxY=min(mini?225:219,max(y+ry,oldX>=0?oldY+ry:y+ry));
  uint16_t palette[11];
  for(uint8_t i=0;i<11;i++)palette[i]=mascotColor(i,bg);
  for(int row=minY;row<=maxY;row++){
    int run=-1;uint8_t tone=0;
    for(int px=minX;px<=maxX+1;px++){
      uint8_t newer=0,older=0;
      if(px<=maxX){
        newer=spriteAt(px,row,x,y,frame,mood,serial,mini);
        if(oldX>=0)older=spriteAt(px,row,oldX,oldY,oldFrame,oldMood,serial,mini);
      }
      if(px<=maxX && newer!=older && (run<0||newer==tone)){
        if(run<0){run=px;tone=newer;}
      }else{
        if(run>=0){gfx->drawFastHLine(run,row,px-run,palette[tone]);run=-1;}
        if(px<=maxX && newer!=older){run=px;tone=newer;}
      }
    }
    if((row&15)==0)yield();
  }
}

} // namespace

void MascotMode::begin(const Settings& s){
  (void)s;
  painted_=false;active_=false;selected_=false;dedicated_=false;
  carouselActive_=false;carouselQuoteUntilMs_=0;
  phase_=SPEAK;phaseUntilMs_=0;roamStartMs_=0;
  speechSerial_=0;prevX_=-1;prevY_=-1;
  prevSmallX_=-1;prevSmallY_=-1;prevFrame_=0;prevSmallFrame_=0;
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
  selected_=true;painted_=false;prevX_=-1;prevSmallX_=-1;nextAnimationMs_=0;
}
void MascotMode::tick(const Settings& s,bool dedicated){
  const uint32_t now=millis();
  if(!s.mascot.enabled){
    active_=false;
    dedicated_=false;
    carouselActive_=false;
    return;
  }
  // A carousel page is NOT a dedicated Tamagotchi session.
  // Earlier the global tick(false) and service tick(true) fought every loop:
  // dedicated_ flipped false/true at ~5ms and chose a new phrase each time.
  // Let service() handle one stable timed quote while its carousel page is up.
  if(s.mode==MODE_CAROUSEL){
    active_=false; dedicated_=false;
    return;
  }
  carouselActive_=false;
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
  if(dedicated_){
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
    untilMs_=now+kPopupMs;
    // On a normal clock the motivation overlay is brief and optional.
    // The 60s/30s cycle belongs only to the dedicated Aviafriend mode.
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
  painted_=true;nextAnimationMs_=0;prevSmallX_=-1;
}
void MascotMode::drawSmallCharacter(const Settings& s){
  (void)s;
  const uint32_t frame=millis()/230UL;
  const int bob=(int)((frame+mood_)%5u)-2;
  const int cx=94+bob,cy=198+((mood_==2&&(frame&1u))?-2:0);
  drawSpriteDelta(gfxDev(),cx,cy,frame,mood_,prevSmallX_,prevSmallY_,
                  prevSmallFrame_,prevSmallMood_,speechSerial_,true,backdrop());
  prevSmallX_=cx;prevSmallY_=cy;
  prevSmallFrame_=frame;prevSmallMood_=mood_;
}
// The enlarged sprite moves around a perimeter with NO black erase rectangle.
// The larger mascot moves along a rectangle following the SCREEN PERIMETER.
// Position is derived from elapsed time, not repeated accumulated increments,
// so web requests do not freeze the animation timeline.
void MascotMode::drawBigCharacter(int cx,int cy,uint32_t frame){
  drawSpriteDelta(gfxDev(),cx,cy,frame,mood_,prevX_,prevY_,
                  prevFrame_,prevMood_,speechSerial_,false,C_BLACK);
  prevX_=cx;prevY_=cy;prevFrame_=frame;prevMood_=mood_;
}
void MascotMode::drawRoaming(const Settings& s){
  (void)s;
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  if(!painted_){
    gfx->fillScreen(C_BLACK);
    ukrtext::centered(gfx,"АВІАДРУГ",3,1,sky());
    ukrtext::centered(gfx,"НАСТУПНА ФРАЗА ЗА 30 С",228,1,C_GRAY);
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
  drawBigCharacter(x,y,frame);
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
  if(s.mode==MODE_CAROUSEL){
    // A carousel slot keeps changing quotes but never enters the 30s roaming
    // phase. Crucially: do not call tick(s,true) here!
    const uint32_t now=millis();
    if(!carouselActive_ || (int32_t)(now-carouselQuoteUntilMs_)>=0){
      carouselActive_=true;
      choose(s);
      carouselQuoteUntilMs_=now+kCarouselQuoteMs;
    }
    renderOverlay(s);
    return;
  }
  // main loop already advances the standalone session via tick(s,true).
  if(phase_==ROAM)drawRoaming(s);
  else renderOverlay(s);
}
