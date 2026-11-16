// Air Alerts Ukraine: one compact API request for the map and selected-region
// notifications. Schematic map geometry is authored for the 240x240 screen.
// API docs: https://devs.alerts.in.ua/packages
#include "AlertsMode.h"
#include "Gfx.h"
#include "Net.h"
#include "Clock.h"
#include "Platform.h"
#include "UkrText.h"
#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <memory>
#include <string.h>

AlertsMode g_alertsMode;

// Exact order of the 27 characters returned by the by_oblast API.
static const uint16_t kUids[27] = {
  29,8,4,9,28,10,11,12,13,31,14,15,16,27,17,18,19,5,30,20,21,22,23,3,24,26,25
};
// Schematic centres (not administrative boundary coordinates) of the 27 units.
// The display labels it as a schematic, not a precise district-by-district map.
static uint8_t kDots[27][2] = {
  {143,187},{42,70},{91,116},{165,125},{190,144},{89,78},
  {27,132},{162,151},{52,124},{111,89},{112,95},{130,135},
  {208,119},{39,100},{126,161},{101,173},{150,103},{64,75},
  {135,199},{166,70},{64,112},{191,98},{146,168},{83,108},
  {124,114},{84,137},{129,70}
};
// Outline is an intentionally simplified vector silhouette of Ukraine.
// It is NOT a political/raion boundary map.
static uint8_t kOutline[][2] = {
  {13,74},{31,68},{45,55},{70,57},{83,64},{103,51},{129,43},
  {157,46},{174,58},{184,67},{209,75},{225,87},{222,111},
  {228,128},{214,142},{194,151},{183,162},{165,169},{155,180},
  {157,191},{151,202},{140,207},{130,202},{130,190},{118,181},
  {111,169},{102,177},{94,170},{88,151},{75,144},{60,147},
  {53,139},{42,136},{27,125},{13,119},{10,97}
};

static int regionIndex(uint16_t uid) {
  for(int i=0;i<27;i++) if(kUids[i]==uid) return i;
  return -1;
}
static bool isActive(char status) { return status=='R'||status=='Y'||status=='P'; }
static bool validStatus(char c) { return c=='N'||c=='R'||c=='Y'||c=='P'; }
static uint16_t colorFor(char c) {
  switch(c){
    case 'N': return C_GREEN;
    case 'R': return C_RED;
    case 'Y': return C_YELLOW;
    case 'P': return gfxTint(0xFC00);
    default: return C_DGRAY;
  }
}
void AlertsMode::begin(const Settings& s) { invalidate(s); }
void AlertsMode::invalidate(const Settings&) {
  // A change to region, token, or settings invalidates all previous data.
  nextPollMs_=0;lastOkMs_=0;lastHttp_=0;
  state_='?';failed_=true;hadValid_=false;
  memset(oblasts_,0,sizeof(oblasts_));
  overlayUntil_=0;needRender_=true;error_="очікування даних";
}
uint32_t AlertsMode::ageSec() const {
  return lastOkMs_ ? (millis()-lastOkMs_)/1000UL : 0;
}
bool AlertsMode::fresh(const Settings& s) const {
  return lastOkMs_ && !failed_ && ageSec()<=(uint32_t)s.alerts.pollSec*3UL;
}
void AlertsMode::startOverlay(bool alert) {
  overlayAlert_=alert;
  overlayUntil_=millis()+12000UL;
  overlayDrawnAt_=0;
}
bool AlertsMode::fetch(const Settings& s) {
  // Theme graphics may temporarily consume the contiguous heap needed by TLS.
  // Fail closed rather than treating a failed fetch as an all-clear.
  if(platformMaxFreeBlock()<16000){error_="замало пам'яті";return false;}
  std::unique_ptr<SecureClient> client(platformMakeSecureClient(4096));
  if(!client){error_="помилка TLS";return false;}
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(s.httpTimeout);
  http.setReuse(false);
  if(!http.begin(*client, F("https://api.alerts.in.ua/v2/iot/active_air_raid_alerts_by_oblast.json"))){
    error_="немає з'єднання";return false;
  }
  http.addHeader("Authorization", "Bearer " + s.alerts.token);
  http.addHeader("Accept","application/json");
  http.setUserAgent("smalltv-alerts-ua/0.2");
  lastHttp_=http.GET();
  if(lastHttp_!=HTTP_CODE_OK){
    error_=lastHttp_==401?"недійсний токен":
           lastHttp_==429?"ліміт API":"помилка API";
    http.end();return false;
  }
  JsonDocument doc;
  DeserializationError err=deserializeJson(doc,http.getStream());
  http.end();
  if(err||!doc.is<const char*>()){error_="помилка JSON";return false;}
  const char* p=doc.as<const char*>();
  if(!p||strlen(p)!=27){error_="помилка довжини";return false;}
  for(int i=0;i<27;i++)if(!validStatus(p[i])){
    error_="помилка статусу";return false;
  }
  int index=regionIndex(s.alerts.uid);
  if(index<0){error_="оберіть область";return false;}

  const char prev=state_;
  const char now=p[index];
  memcpy(oblasts_,p,27);
  oblasts_[27]=0;
  state_=now;
  error_="";
  if(s.alerts.notify){
    if(!hadValid_ && isActive(now)) startOverlay(true);
    else if(hadValid_){
      if(!isActive(prev) && isActive(now)) startOverlay(true);
      else if(isActive(prev) && now=='N') startOverlay(false);
    }
  }
  hadValid_=true;
  return true;
}
void AlertsMode::poll(const Settings& s) {
  if(!s.alerts.enabled||!s.alerts.token.length()){
    const char* reason=s.alerts.enabled?"введіть API-токен":"моніторинг вимкнено";
    // The clock/map is serviced every ~5 ms. Repaint only on a change,
    // not on every tick when there is no token.
    if(state_!='?' || !failed_ || strcmp(error_,reason)!=0) needRender_=true;
    state_='?';failed_=true;error_=reason;
    return;
  }
  if(!netConnected()){
    if(!failed_ || strcmp(error_,"немає Wi-Fi")!=0) needRender_=true;
    failed_=true;error_="немає Wi-Fi";return;
  }
  if(nextPollMs_&&(int32_t)(millis()-nextPollMs_)<0)return;
  nextPollMs_=millis()+(uint32_t)s.alerts.pollSec*1000UL;
  if(fetch(s)){lastOkMs_=millis()|1UL;failed_=false;}
  else {
    failed_=true;
    if(lastHttp_==429)nextPollMs_=millis()+120000UL;
  }
  needRender_=true;
}

// Fill the rough country silhouette using a scanline; no stored bitmap or
// framebuffer, only a couple of bytes of stack per edge.
static void drawCountry(Arduino_GFX* gfx) {
  constexpr int count=sizeof(kOutline)/sizeof(kOutline[0]);
  for(int y=43;y<=209;y++){
    int cuts[24],n=0;
    for(int i=0;i<count;i++){
      int j=(i+1)%count;
      int x1=kOutline[i][0];
      int y1=kOutline[i][1];
      int x2=kOutline[j][0];
      int y2=kOutline[j][1];
      if(((y1<=y)&&(y<y2))||((y2<=y)&&(y<y1))){
        if(n<24)cuts[n++]=x1+(y-y1)*(x2-x1)/(y2-y1);
      }
    }
    for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)
      if(cuts[j]<cuts[i]){int t=cuts[i];cuts[i]=cuts[j];cuts[j]=t;}
    for(int i=0;i+1<n;i+=2)
      gfx->drawFastHLine(cuts[i],y,cuts[i+1]-cuts[i]+1,gfxTint(0x10A2));
    if((y&15)==0)yield();
  }
  for(int i=0;i<count;i++){
    int j=(i+1)%count;
    gfx->drawLine(kOutline[i][0],kOutline[i][1],
                  kOutline[j][0],kOutline[j][1],C_GRAY);
  }
}
void AlertsMode::render(const Settings& s){
  Arduino_GFX* gfx=gfxDev();if(!gfx)return;
  gfx->fillScreen(C_BLACK);
  ukrtext::centered(gfx,"КАРТА ТРИВОГ",9,2,C_WHITE);
  drawCountry(gfx);
  bool usable=fresh(s);
  for(int i=0;i<27;i++){
    uint8_t x=kDots[i][0];
    uint8_t y=kDots[i][1];
    uint16_t c=usable?colorFor(oblasts_[i]):C_DGRAY;
    gfx->fillCircle(x,y,i==9||i==18?3:5,c);
    if(kUids[i]==s.alerts.uid){
      gfx->drawCircle(x,y,8,C_WHITE);
      gfx->drawCircle(x,y,9,C_WHITE);
    }
  }
  // Compact status banner. Never use green when data is invalid/stale.
  const char* status="НЕМАЄ ДАНИХ";
  if(usable)switch(state_){
    case 'N':status="БЕЗ ТРИВОГИ";break;
    case 'R':status="ТРИВОГА";break;
    case 'Y':status="ЗАГРОЗА";break;
    case 'P':status="ЧАСТКОВА";break;
  }
  gfx->fillRoundRect(8,211,224,25,6,gfxTint(0x2104));
  ukrtext::centered(gfx,status,218,2,usable?colorFor(state_):C_YELLOW);
}
void AlertsMode::renderOverlay(const Settings& s){
  // Skip re-painting every 5 ms; overlay holds its screen between refreshes.
  if(overlayDrawnAt_ && millis()-overlayDrawnAt_<1000)return;
  overlayDrawnAt_=millis()|1UL;
  Arduino_GFX* gfx=gfxDev(); if(!gfx)return;
  uint16_t col=overlayAlert_?C_RED:C_GREEN;
  gfx->fillScreen(C_BLACK);
  gfx->fillRoundRect(8,16,224,194,16,col);
  gfx->fillRoundRect(13,21,214,184,12,C_BLACK);
  ukrtext::centered(gfx,overlayAlert_?"УВАГА":"ВІДБІЙ",42,3,col);
  ukrtext::centered(gfx,overlayAlert_?"ПОВІТРЯНА":"ТРИВОГУ",96,2,C_WHITE);
  ukrtext::centered(gfx,overlayAlert_?"ТРИВОГА":"СКАСОВАНО",120,2,C_WHITE);
  // Region nickname may be Cyrillic (rendered in uppercase).
  ukrtext::centered(gfx,s.alerts.label.c_str(),168,1,C_GRAY);
  ukrtext::centered(gfx,"ПЕРЕВІРТЕ СПОВІЩЕННЯ",220,1,C_YELLOW);
}
void AlertsMode::service(const Settings& s){
  if(needRender_||millis()-lastRenderMs_>15000UL){
    render(s);needRender_=false;lastRenderMs_=millis();
  }
}
