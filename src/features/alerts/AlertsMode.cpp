// Air Alerts Ukraine: one compact API request for the map and selected-region
// notifications. Schematic map geometry is authored for the 240x240 screen.
// API docs: https://devs.alerts.in.ua/packages
#include "AlertsMode.h"
#include "Gfx.h"
#include "Net.h"
#include "Clock.h"
#include "Platform.h"
#include "UkrText.h"
#include "AlertMap.h"
#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <memory>
#include <string.h>

AlertsMode g_alertsMode;

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
  overlayUntil_=0;needRender_=true;frameReady_=false;
  paintedFresh_=false;error_="очікування даних";
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
  int index=alertMapRegionIndex(s.alerts.uid);
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
  const bool wasFresh=fresh(s);
  const char oldRegion=state_;
  const bool wasFailed=failed_;
  char oldStatuses[28];
  memcpy(oldStatuses,oblasts_,sizeof(oldStatuses));
  nextPollMs_=millis()+(uint32_t)s.alerts.pollSec*1000UL;
  if(fetch(s)){lastOkMs_=millis()|1UL;failed_=false;}
  else{
    failed_=true;
    if(lastHttp_==429)nextPollMs_=millis()+120000UL;
  }
  // The network may return the same status every minute. Don't redraw unless
  // the displayed information (including freshness) really changed.
  if(wasFresh!=fresh(s) || oldRegion!=state_ ||
     wasFailed!=failed_ || memcmp(oldStatuses,oblasts_,sizeof(oldStatuses))!=0)
    needRender_=true;
}

// Full panel initialization occurs only on a mode switch or after an overlay.
// Routine polling never clears the entire frame.
void AlertsMode::render(const Settings& s) {
  Arduino_GFX* gfx=gfxDev();
  if(!gfx)return;
  const bool usable=fresh(s);
  alertMapDraw(gfx,oblasts_,usable,s.alerts.uid,!frameReady_);

  const char* label="НЕМАЄ ДАНИХ";
  if(usable) {
    switch(state_){
      case 'N':label="БЕЗ ТРИВОГИ";break;
      case 'R':label="ТРИВОГА";break;
      case 'Y':label="ЗАГРОЗА";break;
      case 'P':label="ЧАСТКОВА";break;
    }
  }
  // Bottom badge updates without clearing any other part of the map.
  uint8_t size=ukrtext::textWidth(label,2)<=216?2:1;
  ukrtext::centered(gfx,label,224,size,usable?(state_=='R'?C_RED:(state_=='Y'||state_=='P')?C_YELLOW:C_WHITE):C_GRAY);
  paintedFresh_=usable;
  frameReady_=true;
}

void AlertsMode::renderOverlay(const Settings& s){
  // This is a static notification: painting it repeatedly caused visible 1 Hz blinking.
  if(overlayDrawnAt_)return;
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
void AlertsMode::service(const Settings& s) {
  // Time-based redraws of an unchanged screen caused the original flicker.
  // Validity transitions (stale data) still force an immediate grey map.
  if(needRender_ || (frameReady_ && paintedFresh_!=fresh(s))) {
    render(s);
    needRender_=false;
  }
}
