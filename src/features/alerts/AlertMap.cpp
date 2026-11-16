#include "AlertMap.h"
#include "Gfx.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <string.h>

// Official API display order:
// https://devs.alerts.in.ua/  /v2/iot/active_air_raid_alerts_by_oblast.json
static const uint16_t kUids[27] = {
  29,8,4,9,28,10,11,12,13,31,14,15,16,27,17,18,19,5,30,20,21,22,23,3,24,26,25
};

// Approximate region centres in the schematic silhouette. All territories,
// including Crimea and both separately monitored cities, remain represented.
// Avoid PROGMEM sub-byte reads: the previous ESP8266 exception was triggered
// in the Cyrillic renderer on Flash-backed unaligned data.
static uint8_t kCentres[27][2] = {
  {142,186},{44,72},{88,117},{165,124},{195,146},{93,79},
  {30,141},{172,153},{52,125},{119,89},{115,96},{132,134},
  {211,119},{43,99},{128,159},{97,171},{148,102},{65,76},
  {131,197},{168,72},{67,111},{194,100},{146,172},{81,103},
  {126,111},{86,143},{132,71}
};

// Country outline is schematic (not an administrative/geodetic map).
// Larger map with clear southern peninsula and distinct eastern/western contours.
static uint8_t kOutline[][2] = {
  {11,86},{20,77},{30,64},{42,61},{53,66},{65,58},{83,64},
  {96,57},{109,57},{125,51},{140,46},{153,49},{161,56},
  {176,58},{194,68},{204,70},{216,79},{225,88},
  {228,99},{223,111},{230,123},{223,136},{211,143},
  {200,150},{187,152},{177,161},{164,166},{153,177},
  {152,186},{159,197},{154,205},{145,207},{140,200},
  {134,196},{130,183},{120,174},{111,174},{102,180},
  {90,172},{77,171},{67,163},{54,155},{44,152},{35,151},
  {28,140},{21,132},{13,118},{11,106},{17,96}
};

constexpr uint8_t kRegionCount = 27;
constexpr uint8_t kStartY = 45;
constexpr uint8_t kEndY = 207;
constexpr uint8_t kMapW = 240;
constexpr uint8_t kOutlineCount = sizeof(kOutline)/sizeof(kOutline[0]);

int alertMapRegionIndex(uint16_t uid) {
  for (int i=0;i<kRegionCount;i++) if(kUids[i]==uid) return i;
  return -1;
}

// Region colouring is intentionally muted and consistent. A saturated ring
// and bottom status banner draw attention to the specifically chosen region.
static uint16_t regionFill(char status) {
  switch(status) {
    case 'R': return gfxTint(0xA145); // red
    case 'Y': return gfxTint(0x9BC8); // amber/yellow
    case 'P': return gfxTint(0x83A6); // orange
    case 'N': return gfxTint(0x1D2B); // subdued green/teal
    default:  return gfxTint(0x2945); // unknown grey-blue
  }
}
static uint16_t regionText(char status) {
  switch(status){
    case 'R':return C_RED;
    case 'Y':return C_YELLOW;
    case 'P':return gfxTint(0xFC00);
    case 'N':return C_GREEN;
    default:return C_YELLOW;
  }
}

// Nearest-centre cells approximate oblast territories. No per-frame heap
// allocation or offscreen framebuffer: the raster is drawn in horizontal runs.
static uint8_t nearestRegion(int x,int y){
  uint32_t best=0xFFFFFFFFul;
  uint8_t result=0;
  for(uint8_t i=0;i<kRegionCount;i++){
    const int dx=x-kCentres[i][0],dy=y-kCentres[i][1];
    uint32_t dist=(uint32_t)(dx*dx+dy*dy);
    if(dist<best){best=dist;result=i;}
  }
  return result;
}
static uint8_t rowIntersections(int y,int out[12]){
  uint8_t n=0;
  for(uint8_t i=0;i<kOutlineCount;i++){
    const uint8_t j=(i+1)%kOutlineCount;
    const int x1=kOutline[i][0],y1=kOutline[i][1];
    const int x2=kOutline[j][0],y2=kOutline[j][1];
    if(((y1<=y)&&(y<y2))||((y2<=y)&&(y<y1))){
      if(n<12)out[n++]=x1+(y-y1)*(x2-x1)/(y2-y1);
    }
  }
  for(uint8_t i=0;i<n;i++)for(uint8_t j=i+1;j<n;j++)
    if(out[j]<out[i]){int t=out[i];out[i]=out[j];out[j]=t;}
  return n;
}

// Draw status areas without erasing the panel. Draw only on a new frame,
// when the 27 statuses change, or when validity changes. This is deliberately
// unlike a full-frame bitmap refresh: no black flash on an API poll.
void alertMapDraw(Arduino_GFX* gfx,const char statuses[28],
                  bool usable,uint16_t selectedUid,bool fullFrame){
  if(!gfx)return;

  if(fullFrame){
    gfx->fillScreen(C_BLACK);
    gfx->fillRect(0,0,kMapW,42,gfxTint(0x0862));
    ukrtext::centered(gfx,"МАПА ТРИВОГ",5,2,C_WHITE);
    // Small compact legend: no large cards competing with the map.
    gfx->fillRoundRect(18,29,8,8,2,C_RED);
    ukrtext::drawAt(gfx,"ТРИВОГА",30,29,1,C_GRAY);
    gfx->fillRoundRect(103,29,8,8,2,C_YELLOW);
    ukrtext::drawAt(gfx,"ЗАГРОЗА",115,29,1,C_GRAY);
    gfx->fillRoundRect(189,29,8,8,2,C_GREEN);
    ukrtext::drawAt(gfx,"БЕЗ",202,29,1,C_GRAY);
  }

  uint8_t prev[kMapW],cur[kMapW];
  memset(prev,255,sizeof(prev));
  const uint16_t border=gfxTint(0x8430);
  for(int y=kStartY;y<kEndY;y++){
    memset(cur,255,sizeof(cur));
    int cuts[12];
    const uint8_t n=rowIntersections(y,cuts);
    for(uint8_t span=0;span+1<n;span+=2){
      const int left=max(0,cuts[span]);
      const int right=min(239,cuts[span+1]);
      if(right<left)continue;
      uint8_t runRegion=255;
      int runStart=left;
      // Region boundaries come from a labelled nearest-centre tessellation.
      // Rasterise one horizontal run at a time: far fewer SPI transactions
      // than writing individual pixels.
      for(int x=left;x<=right+1;x++){
        uint8_t id=(x<=right)?nearestRegion(x,y):255;
        if(x<=right)cur[x]=id;
        if(id!=runRegion){
          if(runRegion!=255){
            uint16_t color=usable?regionFill(statuses[runRegion]):regionFill('?');
            gfx->drawFastHLine(runStart,y,x-runStart,color);
          }
          runRegion=id;
          runStart=x;
        }
      }
    }
    // One-pixel crisp internal borders. The scanline step creates clean
    // region-shaped patches instead of the previous 27 isolated circles.
    for(int x=12;x<=228;x++){
      if(cur[x]==255)continue;
      if((x>12 && cur[x-1]!=255 && cur[x-1]!=cur[x]) ||
         (prev[x]!=255 && prev[x]!=cur[x]))
        gfx->drawPixel(x,y,border);
    }
    memcpy(prev,cur,sizeof(cur));
    if((y&15)==0)yield();
  }

  // Natural coastline-style outline and the selected monitoring location.
  for(uint8_t i=0;i<kOutlineCount;i++){
    const uint8_t j=(i+1)%kOutlineCount;
    gfx->drawLine(kOutline[i][0],kOutline[i][1],
                  kOutline[j][0],kOutline[j][1],C_GRAY);
  }
  const int sel=alertMapRegionIndex(selectedUid);
  if(sel>=0){
    int x=kCentres[sel][0],y=kCentres[sel][1];
    gfx->fillCircle(x,y,3,C_WHITE);
    gfx->drawCircle(x,y,7,C_WHITE);
    gfx->drawCircle(x,y,8,C_BLACK);
  }
  // Draw a static divider but don't blank the whole map for a status update.
  gfx->drawFastHLine(8,210,224,C_DGRAY);
  gfx->fillRoundRect(6,214,228,24,5,gfxTint(0x10A3));
}
