#include "MascotMode.h"
#include "MotivationPhrases.h"
#include "Gfx.h"
#include "Clock.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <time.h>

MascotMode g_mascotMode;
namespace {
// Durations are saved in MascotSettings and configurable from the web UI.
constexpr uint32_t kPopupMs=15000UL;
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
  // At 3x, keep at most three rows so the enlarged author name has its
  // own reserved area. For longer lines use up to five 2x rows.
  if(n==0||n>3) {
    scale=2;
    n=wrapText(quote,scale,lines);
  }
  if(!n)return;
  const int step=(scale==3)?25:17;
  const int first=(scale==3)?39:34;
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
// Four-point signed-area test, used for elegant swept airplane wings.
static bool triangle(int x,int y,int ax,int ay,int bx,int by,int cx,int cy){
  const int32_t s1=(int32_t)(x-ax)*(by-ay)-(int32_t)(y-ay)*(bx-ax);
  const int32_t s2=(int32_t)(x-bx)*(cy-by)-(int32_t)(y-by)*(cx-bx);
  const int32_t s3=(int32_t)(x-cx)*(ay-cy)-(int32_t)(y-cy)*(ax-cx);
  return !((s1<0||s2<0||s3<0)&&(s1>0||s2>0||s3>0));
}
static bool quadrilateral(int x,int y,
                          int ax,int ay,int bx,int by,int cx,int cy,int dx,int dy){
  return triangle(x,y,ax,ay,bx,by,cx,cy) ||
         triangle(x,y,ax,ay,cx,cy,dx,dy);
}
// A smiling FIXED-WING AIRPLANE: broad swept wings, vertical fin, stabilisers,
// cockpit canopy, nose cone and wheels. No helicopter/drone rotors.
// Palette indices: 0 transparent, 1 dark outline, 2 blue aviation body,
// 3 pearl white, 4 eyes/lines, 5 golden accent, 6 steel, 7 sky highlight,
// 8 windshield, 9 pink cheeks, 10 happy sparks.
static uint8_t mascotPixel(int x,int y,uint32_t frame,uint8_t mood,uint16_t speech){
  if(x<-79||x>79||y<-51||y>53)return 0;
  const int flex=((frame+mood)%8u<4u)?2:0;
  const bool blink=((frame/2u+speech)%29u)==0u;
  const bool grin=((frame+mood)%9u)<7u;
  uint8_t c=0;
  // Cheerful sparkle never obscures the aircraft silhouette.
  if((mood==1||mood==3)&&frame%5u<3u&&disc(x,y,74,-30,3))c=10;
  // Rear horizontal stabilisers, clearly part of the airplane tail.
  if(quadrilateral(x,y,-48,-29,-12,-34,-12,-22,-45,-20) ||
     quadrilateral(x,y,12,-34,48,-29,45,-20,12,-22))c=1;
  if(quadrilateral(x,y,-44,-28,-13,-31,-13,-25,-41,-23) ||
     quadrilateral(x,y,13,-31,44,-28,41,-23,13,-25))c=7;
  // Vertical tail fin rising above the cockpit.
  if(triangle(x,y,-13,-21,0,-50,13,-21))c=1;
  if(triangle(x,y,-8,-22,0,-44,8,-22))c=2;
  if(triangle(x,y,-3,-26,0,-41,3,-26))c=5;

  // Signature swept AIRPLANE WINGS: fixed continuous surfaces, *not rotors*.
  if(quadrilateral(x,y,-77,0,-26,-18,-19,23,-73,25+flex) ||
     quadrilateral(x,y,26,-18,77,0,73,25+flex,19,23))c=1;
  if(quadrilateral(x,y,-73,3,-25,-14,-22,19,-70,22+flex) ||
     quadrilateral(x,y,25,-14,73,3,70,22+flex,22,19))c=2;
  // Two strong colour stripes and bright outer wing tips.
  if(quadrilateral(x,y,-71,8,-35,-7,-34,-2,-69,13) ||
     quadrilateral(x,y,35,-7,71,8,69,13,34,-2))c=7;
  if(quadrilateral(x,y,-74,9,-67,6,-64,22,-72,23) ||
     quadrilateral(x,y,67,6,74,9,72,23,64,22))c=5;

  // Oval fuselage and curved windshield: friendly pilot-like eyes.
  if(rounded(x,y,-34,-30,68,78,23))c=1;
  if(rounded(x,y,-32,-28,64,75,22))c=2;
  if(rounded(x,y,-29,-25,58,69,19))c=3;
  if(ellipse(x,y,0,-9,26,19))c=8;
  if(ellipse(x,y,-17,-17,7,4)||ellipse(x,y,17,-17,7,4))c=3;
  if(rounded(x,y,-21,-32,42,9,4))c=1;
  if(rounded(x,y,-19,-31,38,7,3))c=7;
  if(rounded(x,y,-10,-29,20,2,1))c=3;
  if(disc(x,y,0,-31,2))c=5;

  // Blinking eyes + playful eyebrows. Nose/eyes are in the canopy.
  if(y>=-13&&y<=-11&&((x>=-22&&x<=-10)||(x>=10&&x<=22)))c=2;
  if(blink) {
    if(y>=-4&&y<=-2&&((x>=-21&&x<=-12)||(x>=12&&x<=21)))c=4;
  }else {
    if(ellipse(x,y,-16,-4,5,7)||ellipse(x,y,16,-4,5,7))c=4;
    if(disc(x,y,-18,-7,2)||disc(x,y,14,-7,2))c=3;
  }
  if(ellipse(x,y,-25,12,5,3)||ellipse(x,y,25,12,5,3))c=9;
  if(grin){
    if(y>=14&&y<=22&&abs(x)<=12 && y>=16+abs(x)/5)c=4;
    if(y>=19&&y<=21&&abs(x)<=8)c=9;
  }else if(ellipse(x,y,0,18,10,7)){
    c=4;
    if(y>=21&&ellipse(x,y,0,23,7,3))c=9;
  }
  // Pointed aircraft nose cone and central golden badge.
  if(triangle(x,y,-15,34,0,53,15,34))c=1;
  if(triangle(x,y,-11,35,0,49,11,35))c=2;
  if(triangle(x,y,-7,37,0,46,7,37))c=5;
  if(rounded(x,y,-6,26,12,5,2))c=5;
  // Two little wheels, each with a dark tyre and silver centre.
  if(disc(x,y,-25,42,6)||disc(x,y,25,42,6))c=1;
  if(disc(x,y,-25,42,3)||disc(x,y,25,42,3))c=6;
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
  return mini?mascotPixel(dx*2,dy*2,frame,mood,serial)
             :mascotPixel(dx,dy,frame,mood,serial);
}
// Batch consecutive differing pixels into horizontal monochrome palette runs.
// No framebuffer, fillRect erase, screen flashing or 16-bit canvas allocation.
// The previous per-row drawFastHLine approach issued thousands of tiny SPI
// writes and the user saw a travelling top-to-bottom refresh wave.
// Instead compose the old+new sprite union in a 4-row RGB565 tile and push
// each tile in ONE bitmap transfer. No blank old frame, no full LCD refresh.
// 4*240*2 = 1920 B static memory; avoids ESP8266 stack overflow.
static void drawSpriteDelta(Arduino_GFX* gfx,
                            int x,int y,uint32_t frame,uint8_t mood,
                            int oldX,int oldY,uint32_t oldFrame,uint8_t oldMood,
                            uint16_t serial,bool mini,uint16_t bg) {
  if(!gfx)return;
  constexpr int kTileRows=4;
  static uint16_t tile[240*kTileRows];
  const int rx=mini?40:79,ry=mini?27:53;
  const int minX=max(0,min(x-rx,oldX>=0?oldX-rx:x-rx));
  const int maxX=min(239,max(x+rx,oldX>=0?oldX+rx:x+rx));
  const int minY=max(mini?173:22,min(y-ry,oldX>=0?oldY-ry:y-ry));
  const int maxY=min(mini?226:218,max(y+ry,oldX>=0?oldY+ry:y+ry));
  if(minX>maxX || minY>maxY)return;
  const int width=maxX-minX+1;
  for(int top=minY;top<=maxY;top+=kTileRows) {
    const int height=min(kTileRows,maxY-top+1);
    bool changed=false;
    for(int ty=0;ty<height;ty++) {
      const int sy=top+ty;
      for(int col=0;col<width;col++){
        const int sx=minX+col;
        const uint8_t fresh=spriteAt(sx,sy,x,y,frame,mood,serial,mini);
        const uint8_t stale=oldX>=0?
            spriteAt(sx,sy,oldX,oldY,oldFrame,oldMood,serial,mini):0;
        changed|=fresh!=stale;
        tile[ty*width+col]=mascotColor(fresh,bg);
      }
    }
    if(changed) {
      // Fixed-width color bitmap transfer is many times fewer SPI commands
      // than one-pixel or multi-run drawing.
      gfx->draw16bitRGBBitmap(minX,top,tile,width,height);
    }
    if((top&15)==0)yield();
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
      phaseUntilMs_=now+(uint32_t)s.mascot.speechSec*1000UL;
    }else if((int32_t)(now-phaseUntilMs_)>=0){
      if(phase_==SPEAK){
        phase_=ROAM;
        roamStartMs_=now;
        phaseUntilMs_=now+(uint32_t)s.mascot.roamSec*1000UL;
        mood_=(mood_+1u+(speechSerial_%3u))%5u;
        painted_=false;prevX_=-1;nextAnimationMs_=0;
      }else{
        phase_=SPEAK;
        choose(s);
        phaseUntilMs_=now+(uint32_t)s.mascot.speechSec*1000UL;
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
  char quote[160],author[72];
  // Phrase table lives in Flash: always copy into aligned RAM first.
  motivation::copy(selectedIndex_,quote,sizeof(quote),author,sizeof(author));
  drawQuote(gfx,quote);
  // Display ACTUAL author at a legible size; adaptations are honestly marked
  // in a separate, smaller subtitle rather than squeezing the author to 1x.
  String credit(author);
  const int marker=credit.indexOf(" • ");
  const bool adapted=marker>=0;
  if(adapted)credit.remove(marker);
  uint8_t authorSize=ukrtext::textWidth(credit.c_str(),2)<=216?2:1;
  ukrtext::centered(gfx,credit.c_str(),126,authorSize,gfxTint(0x2539));
  if(adapted)ukrtext::centered(gfx,"ПЕРЕКАЗ",143,1,C_GRAY);
  gfx->drawFastHLine(14,229,210,sky());
  painted_=true;nextAnimationMs_=0;prevSmallX_=-1;
}
void MascotMode::drawSmallCharacter(const Settings& s){
  (void)s;
  const uint32_t frame=millis()/230UL;
  const int bob=(int)((frame+mood_)%5u)-2;
  const int cx=94+bob,cy=199+((mood_==2&&(frame&1u))?-2:0);
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
    ukrtext::centered(gfx,"АВІАДРУГ ГУЛЯЄ",228,1,C_GRAY);
    painted_=true;prevX_=-1;nextAnimationMs_=0;
  }
  const uint32_t now=millis();
  if(nextAnimationMs_ && (int32_t)(now-nextAnimationMs_)<0)return;
  nextAnimationMs_=now+160UL; // ~6 FPS, buffered LCD transfers
  const uint32_t frame=(now-roamStartMs_)/160UL;
  const uint32_t dist=((now-roamStartMs_)/190UL*2UL)%336UL;
  int x=81,y=75;
  if(dist<78UL)x+=(int)dist;
  else if(dist<168UL){x=159;y+=(int)(dist-78UL);}
  else if(dist<246UL){x=159-(int)(dist-168UL);y=165;}
  else {x=81;y=165-(int)(dist-246UL);}
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
      carouselQuoteUntilMs_=now+(uint32_t)s.mascot.speechSec*1000UL;
    }
    renderOverlay(s);
    return;
  }
  // main loop already advances the standalone session via tick(s,true).
  if(phase_==ROAM)drawRoaming(s);
  else renderOverlay(s);
}
