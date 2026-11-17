// Ukraine oblast map, 240x240. White canvas, pale no-alert regions, red/yellow
// alerts. Natural Earth admin-1 polygons (CC0) rasterised in OblastGeometry.h.
// Crimea is explicitly included. Borders are approximate, not cadastral.
#include "AlertMap.h"
#include "OblastGeometry.h"
#include "Gfx.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>

static const uint16_t kUids[27] = {
  29,8,4,9,28,10,11,12,13,31,14,15,16,27,17,18,19,5,30,20,21,22,23,3,24,26,25
};
int alertMapRegionIndex(uint16_t uid){
  for(int i=0;i<27;i++)if(kUids[i]==uid)return i;
  return -1;
}
static uint16_t stateColor(char state,bool usable){
  if(!usable)return gfxTint(0x73AE); // stale = neutral grey, never false safe
  switch(state){
    case 'R':return gfxTint(0xE3EB); // salmon red, alerts.in.ua-like
    case 'Y':return gfxTint(0xFEC0); // clear yellow
    case 'P':return gfxTint(0xFD80); // amber partial alert
    case 'N':return C_WHITE; // no alert: white oblast
    default:return gfxTint(0xD69A);
  }
}
static uint16_t regionColor(char c,bool fresh){return stateColor(c,fresh);}
static void lineRuns(Arduino_GFX* gfx,int y,int first,int stop,uint16_t color){
  for(int i=first;i+1<stop;i+=2)gfx->drawFastHLine(geoBorder(i),y,geoBorder(i+1),color);
}
void alertMapDraw(Arduino_GFX* gfx,const char statuses[28],
                  bool fresh,uint16_t selectedUid,bool fullFrame){
  if(!gfx)return;
  const uint16_t ink=C_WHITE,border=gfxTint(0x8430);
  if(fullFrame) {
    gfx->fillScreen(C_BLACK);
    ukrtext::centered(gfx,"ТРИВОГИ УКРАЇНИ",5,2,ink);
    gfx->fillRect(13,27,8,8,stateColor('N',true));
    gfx->drawRect(13,27,8,8,border);
    ukrtext::drawAt(gfx,"НЕМАЄ",24,27,1,ink);
    gfx->fillRect(90,27,8,8,stateColor('Y',true));
    ukrtext::drawAt(gfx,"ЖОВТА",101,27,1,ink);
    gfx->fillRect(164,27,8,8,stateColor('R',true));
    ukrtext::drawAt(gfx,"ЧЕРВОНА",175,27,1,ink);
  }
  // Render each administrative zone as compact horizontal colored runs.
  // No framebuffer and no full-screen blank between API updates.
  for(int y=kGeoStartY;y<=kGeoEndY;y++){
    int row=y-kGeoStartY;
    for(int i=geoRow(row);i<geoRow(row+1);i+=3){
      uint8_t x=geoRun(i),length=geoRun(i+1),id=geoRun(i+2);
      uint16_t color=regionColor(statuses[id],fresh);
      gfx->drawFastHLine(x,y,length,color);
    }
    if((row&15)==0)yield();
  }
  // Thin, neutral regional borders, including coastline.
  for(int y=kGeoStartY;y<=kGeoEndY;y++){
    int row=y-kGeoStartY;
    lineRuns(gfx,y,geoBorderRow(row),geoBorderRow(row+1),border);
  }
  // Location indicator uses only the selected region UID, not invented coordinates.
  const int region=alertMapRegionIndex(selectedUid);
  if(region>=0){
    // Approximate marker positions on the 240px map, only for focus.
    static const uint8_t kMarker[27][2]={
      {142,184},{43,70},{88,106},{164,125},{194,143},{91,78},
      {30,135},{165,148},{49,121},{116,90},{110,99},{130,127},
      {204,115},{42,96},{129,150},{96,167},{147,105},{65,78},
      {145,200},{164,72},{66,109},{191,98},{148,163},{83,107},
      {128,113},{86,140},{132,73}
    };
    int x=kMarker[region][0],y=kMarker[region][1];
    gfx->drawCircle(x,y,6,C_BLACK);
    gfx->fillCircle(x,y,2,C_WHITE);
  }
  gfx->fillRect(0,220,240,20,C_BLACK);
  gfx->drawFastHLine(8,220,224,border);
}
