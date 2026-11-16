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

// Procedural 3/4-view airplane. No rotor, drone arms or helicopter hub.
// The shaded wings, tapered fuselage and elevated tail make its silhouette
// recognisably fixed-wing even at 240x240. All graphics are computed on demand
// from a tiny palette rather than storing large animation frames in RAM.
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
// Palette: 0 transparency, 1 contour, 2 blue, 3 ivory, 4 facial ink,
// 5 gold, 6 steel, 7 blue highlight, 8 canopy, 9 pink, 10 sparkle,
// 11 navy shadow, 12 mid-blue side, 13 white reflection.
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
// Signature swept AIRPLANE WINGS are real attached lifting surfaces,
// with a contrasting underside, wing-tip lights and movable ailerons.
// The tail is physically behind the fuselage, not a set of propeller arms.
static uint8_t mascotPixel(int x,int y,uint32_t frame,uint8_t mood,
                           uint16_t speech,int8_t yaw) {
  if(x<-79||x>79||y<-57||y>59)return 0;
  const int flex=((frame+mood)%12u<6u)?2:0;
  const bool blink=((frame/2u+speech)%29u)==0u;
  const bool grin=((frame+mood)%9u)<7u;
  const int farWing=(yaw/10);  // opposing wing sweep gives a 3-D turn illusion
  uint8_t c=0;

  // Small tailplane, subtly separated from the main wings by the body.
  if(quadrilateral(x,y,-43,-30,-12,-39,-11,-28,-44,-23) ||
     quadrilateral(x,y,12,-39,43,-30,44,-23,11,-28))c=1;
  if(quadrilateral(x,y,-39,-29,-13,-36,-12,-29,-39,-25) ||
     quadrilateral(x,y,13,-36,39,-29,39,-25,12,-29))c=7;
  if(triangle(x,y,-13,-23,3,-55,13,-23))c=1;
  if(triangle(x,y,-9,-25,3,-49,10,-24))c=12;
  if(triangle(x,y,-2,-28,3,-48,5,-28))c=5;
  if(disc(x,y,4,-40,2))c=13;

  // Wing thickness/underside first: low-slung edges create volume.
  if(quadrilateral(x,y,-78,4,-25,-13,-19,20,-73,31+flex) ||
     quadrilateral(x,y,25,-13,78,4,73,31+flex,19,20))c=1;
  if(quadrilateral(x,y,-74,8,-25,-9,-20,23,-70,27+flex) ||
     quadrilateral(x,y,25,-9,74,8,70,27+flex,20,23))c=11;
  // Left and right top surfaces have different perspective when turning.
  if(quadrilateral(x,y,-76,-3-farWing,-27,-19,-18,18,-73,21+flex-farWing) ||
     quadrilateral(x,y,27,-19,76,-3+farWing,73,21+flex+farWing,18,18))c=1;
  if(quadrilateral(x,y,-72,0-farWing,-26,-15,-22,16,-69,18+flex-farWing) ||
     quadrilateral(x,y,26,-15,72,0+farWing,69,18+flex+farWing,22,16))c=2;
  // Glints and inset ailerons highlight the airfoil, not a rotor.
  if(quadrilateral(x,y,-68,2-farWing,-36,-10,-33,-5,-67,7-farWing) ||
     quadrilateral(x,y,36,-10,68,2+farWing,67,7+farWing,33,-5))c=7;
  if(quadrilateral(x,y,-70,16-farWing,-46,9,-43,12,-69,21-farWing) ||
     quadrilateral(x,y,46,9,70,16+farWing,69,21+farWing,43,12))c=12;
  if(triangle(x,y,-74,1-farWing,-65,0-farWing,-69,20-farWing))c=5;
  if(triangle(x,y,74,1+farWing,65,0+farWing,69,20+farWing))c=5;
  if(disc(x,y,-72,7-farWing,2))c=9; // port wing tip
  if(disc(x,y,72,7+farWing,2))c=7;  // starboard wing tip

  // Thick oval fuselage: contour, shadow side, glossy forward skin.
  if(rounded(x,y,-32,-29,64,77,23))c=1;
  if(rounded(x,y,-30,-27,60,74,22))c=11;
  if(rounded(x,y,-28,-27,55,71,21))c=2;
  if(rounded(x,y,-25,-24,46,67,19))c=3;
  if(ellipse(x,y,-13,12,8,25))c=13;
  if(ellipse(x,y,22,10,4,24))c=12;

  // Curved windshield containing a cheerful face: bevel + reflections.
  if(ellipse(x,y,0,-7,26,20))c=1;
  if(ellipse(x,y,0,-8,24,18))c=8;
  if(ellipse(x,y,-15,-19,7,4)||ellipse(x,y,15,-19,7,4))c=13;
  if(quadrilateral(x,y,-15,-22,-5,-24,0,-21,-14,-17))c=13;
  // Eyebrows, blinking eyelids, pupils, small directional glints.
  if(y>=-12&&y<=-11&&((x>=-21&&x<=-10)||(x>=10&&x<=21)))c=4;
  if(blink) {
    if(y>=-5&&y<=-3&&((x>=-21&&x<=-11)||(x>=11&&x<=21)))c=4;
  }else{
    if(ellipse(x,y,-16,-4,5,7)||ellipse(x,y,16,-4,5,7))c=4;
    if(disc(x,y,-18,-7,2)||disc(x,y,14,-7,2))c=3;
  }
  if(ellipse(x,y,-24,10,4,3)||ellipse(x,y,24,10,4,3))c=9;
  if(grin){
    if(y>=13&&y<=21&&abs(x)<=12&&y>=15+abs(x)/5)c=4;
    if(y>=19&&y<=21&&abs(x)<=8)c=9;
  }else if(ellipse(x,y,0,18,9,6)){
    c=4;
    if(y>=21&&ellipse(x,y,0,22,7,3))c=9;
  }

  // Raised canopy roof, rivets and centre stripe.
  if(rounded(x,y,-20,-31,40,8,4))c=1;
  if(rounded(x,y,-18,-30,36,6,3))c=12;
  if(rounded(x,y,-7,-29,14,2,1))c=13;
  if(disc(x,y,-21,0,1)||disc(x,y,21,0,1))c=5;
  if(rounded(x,y,-5,25,10,5,2))c=5;
  // Tapered three-layer nose cone with visibly curved cheeks.
  if(triangle(x,y,-16,34,0,59,16,34))c=1;
  if(triangle(x,y,-12,35,0,55,12,35))c=2;
  if(triangle(x,y,-8,38,-1,52,2,38))c=13;
  if(triangle(x,y,2,40,0,55,10,38))c=5;
  if(disc(x,y,0,52,3))c=6;

  // Landing struts and wheels as airplane undercarriage (not drone rotors).
  if(rounded(x,y,-29,35,4,10,1)||rounded(x,y,25,35,4,10,1))c=6;
  if(disc(x,y,-27,44,7)||disc(x,y,27,44,7))c=1;
  if(disc(x,y,-27,44,4)||disc(x,y,27,44,4))c=6;
  if(disc(x,y,-28,43,2)||disc(x,y,26,43,2))c=13;
  if((mood==1||mood==3)&&frame%7u<4u&&disc(x,y,75,-40,3))c=10;
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
    case 11:return gfxTint(0x112B); // deep underside shadow
    case 12:return gfxTint(0x35BA); // dimensional blue side
    case 13:return gfxTint(0xF7BF); // specular highlight
    default:return gfxTint(0x0555);
  }
}
// A 20-preset flight choreography is selected anew after every quote.
// No large frame sequence, camera, 3-D library or heap bitmap is needed.
// Scale, yaw compression and banking simulate a volumetric turn of a 3-D plane.
struct PlanePose {
  int16_t scale=256,shiftX=0,shiftY=0,invX=256,invY=256;
  int8_t bank=0,yaw=0;
};
static int sine16(uint32_t step) {
  static const int8_t wave[16]={0,12,23,30,32,30,23,12,0,-12,-23,-30,-32,-30,-23,-12};
  return wave[step&15u];
}
static PlanePose flightPose(uint32_t frame,uint8_t pattern,bool mini) {
  PlanePose p;
  if(mini){p.scale=128;p.invX=512;p.invY=512;return p;}
  const int a=sine16(frame/2u),b=sine16(frame/2u+4u);
  const int c=sine16(frame/3u+5u),d=sine16(frame/4u+8u);
  p.scale=318;
  // 20 named kinds of flight: side pass, climb, dip, figure-eight,
  // banked circles, barrel-turn illusion, approach, victory loop, etc.
  switch(pattern%20u) {
    case 0: p.shiftX=a/2;p.shiftY=b/4;p.bank=a/3;break;       // side pass
    case 1: p.shiftY=-a/2;p.bank=b/3;p.scale+=b/2;break;     // climb
    case 2: p.shiftY=a/2;p.yaw=b/2;break;                    // diving turn
    case 3: p.shiftX=a/2;p.shiftY=(a*c)/70;p.bank=c/2;break; // figure eight
    case 4: p.shiftX=a/3;p.shiftY=b/3;p.bank=a/2;break;      // left orbit
    case 5: p.shiftX=-a/3;p.shiftY=-b/3;p.bank=-a/2;break;   // right orbit
    case 6: p.bank=a;p.yaw=c/2;p.scale+=b/3;break;           // wingover
    case 7: p.shiftY=b/3;p.bank=-a;p.scale-=c/3;break;       // counter-roll
    case 8: p.shiftX=c/3;p.yaw=a;p.bank=b/3;break;           // yaw to port
    case 9: p.shiftX=-c/3;p.yaw=-a;p.bank=-b/3;break;        // yaw to starboard
    case 10:p.shiftY=a/4;p.scale+=b/2;p.bank=c/3;break;      // approach / retreat
    case 11:p.shiftX=a/4;p.shiftY=c/3;p.scale-=b/2;break;    // floating loop
    case 12:p.shiftY=(a*c)/80;p.bank=b/2;p.yaw=a/2;break;    // lazy eight
    case 13:p.shiftX=a/3;p.shiftY=d/2;p.bank=c/2;break;      // wave flight
    case 14:p.shiftY=b/2;p.scale+=a/3;p.yaw=c/2;break;       // buoyant jump
    case 15:p.shiftX=(a>0?12:-12);p.bank=b/2;break;          // side-to-side
    case 16:p.shiftX=a/3;p.shiftY=b/3;p.yaw=c;p.bank=-b/3;break;// banked arc
    case 17:p.shiftX=c/3;p.shiftY=-a/3;p.bank=b/2;p.scale+=a/3;break;
    case 18:p.shiftY=c/2;p.yaw=b;p.bank=a/3;break;           // corkscrew view
    default:p.shiftX=d/3;p.shiftY=b/3;p.bank=a/2;p.yaw=c/2; // victory dance
  }
  p.scale=(int16_t)constrain((int)p.scale,289,339);
  p.shiftX=(int16_t)constrain((int)p.shiftX,-8,8);
  p.shiftY=(int16_t)constrain((int)p.shiftY,-18,18);
  // Precompute reciprocals ONCE per frame (not for every LCD pixel).
  p.invX=(int16_t)(16777216L/((int32_t)p.scale*(256+p.yaw)));
  p.invY=(int16_t)(65536L/p.scale);
  return p;
}
static inline uint8_t spriteAt(int sx,int sy,int cx,int cy,
                               uint32_t frame,uint8_t mood,uint16_t serial,
                               const PlanePose& pose) {
  if(cx<0)return 0;
  const int dx=sx-cx,dy=sy-cy;
  // Approximate inverse 3-D banking + perspective with fixed-point integers.
  const int x=(int32_t)(dx*256-dy*pose.bank)*pose.invX/65536L;
  const int y=(int32_t)(dy*256+dx*pose.bank)*pose.invY/65536L;
  return mascotPixel(x,y,frame,mood,serial,pose.yaw);
}
// Persistent 4-row RGB565 staging tile; no full-screen erase between frames.
// Render the union of the old/new positions as a composite BEFORE sending it.
static void drawSpriteDelta(Arduino_GFX* gfx,
                            int x,int y,uint32_t frame,uint8_t mood,
                            int oldX,int oldY,uint32_t oldFrame,uint8_t oldMood,
                            uint16_t serial,bool mini,uint16_t bg,
                            uint8_t pattern=0) {
  if(!gfx)return;
  constexpr int kTileRows=4;
  static uint16_t tile[240*kTileRows];
  const PlanePose freshPose=flightPose(frame,pattern,mini);
  const PlanePose oldPose=flightPose(oldFrame,pattern,mini);
  // Convert the palette once per frame, not for every one of 40,000+ pixels.
  uint16_t palette[14];
  for(uint8_t i=0;i<14;i++)palette[i]=mascotColor(i,bg);
  const int rx=mini?40:109,ry=mini?30:78;
  const int minX=max(0,min(x-rx,oldX>=0?oldX-rx:x-rx));
  const int maxX=min(239,max(x+rx,oldX>=0?oldX+rx:x+rx));
  const int minY=max(mini?173:21,min(y-ry,oldX>=0?oldY-ry:y-ry));
  const int maxY=min(mini?226:218,max(y+ry,oldX>=0?oldY+ry:y+ry));
  if(minX>maxX||minY>maxY)return;
  const int width=maxX-minX+1;
  for(int top=minY;top<=maxY;top+=kTileRows) {
    const int height=min(kTileRows,maxY-top+1);
    bool changed=false;
    for(int ty=0;ty<height;ty++) {
      const int py=top+ty;
      for(int col=0;col<width;col++) {
        const int px=minX+col;
        const uint8_t fresh=spriteAt(px,py,x,y,frame,mood,serial,freshPose);
        const uint8_t stale=oldX>=0?
            spriteAt(px,py,oldX,oldY,oldFrame,oldMood,serial,oldPose):0;
        changed|=fresh!=stale;
        tile[ty*width+col]=palette[fresh<14?fresh:0];
      }
    }
    if(changed)gfx->draw16bitRGBBitmap(minX,top,tile,width,height);
    if(((top-minY)&15)==0)yield();
  }
}

} // namespace

void MascotMode::begin(const Settings& s){
  (void)s;
  painted_=false;active_=false;selected_=false;dedicated_=false;
  carouselActive_=false;carouselQuoteUntilMs_=0;
  phase_=SPEAK;phaseUntilMs_=0;roamStartMs_=0;
  speechSerial_=0;motionVariant_=0;prevX_=-1;prevY_=-1;
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
        // Coprime stride and a small variable step avoid a looping screensaver.
        motionVariant_=(uint8_t)((motionVariant_+7u+(speechSerial_%5u))%20u);
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
                  prevFrame_,prevMood_,speechSerial_,false,C_BLACK,motionVariant_);
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
  nextAnimationMs_=now+180UL; // ~5.5 FPS, larger RGB565 airplane tiles
  const uint32_t frame=(now-roamStartMs_)/180UL;
  const PlanePose pose=flightPose(frame,motionVariant_,false);
  // The standalone plane now fills ~210 pixels of the 240-pixel display.
  // Keep its enlarged wings, fin and nose entirely inside the LCD.
  const int x=120+pose.shiftX,y=122+pose.shiftY;
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
