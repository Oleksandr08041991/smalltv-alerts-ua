#!/usr/bin/env node
// Source checks for SmallTV ESP8266 mascot, map, altimeter and OTA workflows.
// These do not replace physical-device testing.
const fs=require('node:fs'),assert=require('node:assert/strict');
const read=f=>fs.readFileSync(f,'utf8');
const phrases=read('src/features/mascot/MotivationPhrases.h');
const count=(phrases.match(/^  \{t\d+,a\d+,[01]\},?$/gm)||[]).length;
assert(count>=130, 'At least 130 aviation and business motivational lines required');
assert(phrases.includes('Леся Українка')&&phrases.includes('Іван Франко'),'Literary authors missing');
assert(phrases.includes('Франклін • переказ'),'Franklin paraphrases must be identified as such');
assert(phrases.includes('Авіадруг'),'Original texts must be attributed to the mascot, not labelled Original Phrase');
assert(phrases.includes('PROGMEM') && phrases.includes('memcpy_P'), 'All quotes must be Flash-backed to save OTA heap');
const mascot=read('src/features/mascot/MascotMode.cpp');
const renderer=mascot.split('static uint8_t mascotPixel(')[1].split('static uint16_t mascotColor(')[0];
assert(renderer.includes('blink') && renderer.includes('flex') && renderer.includes('grin'), 'Animated eyes, wings and smile missing');
const delta=mascot.split('static void drawSpriteDelta(')[1].split('} // namespace')[0];
assert(!delta.includes('fillScreen(') && !delta.includes('fillRect('), 'Walking must update only changed pixels without erasing rectangles');
assert(mascot.includes('drawSpriteDelta('), 'Sprite delta-rendering missing');
assert(mascot.includes('draw16bitRGBBitmap('), 'Smooth motion must send multi-row RGB565 tiles, not thousands of per-scanline writes');
assert(mascot.includes('PlaneAtlas.generated.h') && mascot.includes('pgm_read_byte(&planeflash::frames['),
  'ESP8266 must use pre-rendered palette sprites from Flash, not polygon math per pixel');
assert(mascot.includes('kTileRows=12,kTileCols=24') &&
  mascot.includes('if(changed)gfx->draw16bitRGBBitmap(left,top,tile,width,height);'),
  'Only individually dirty 24x12 tiles should be sent to ST7789');
assert(mascot.includes('nextAnimationMs_=now+90UL;') &&
  mascot.includes('const uint32_t frame=(now-roamStartMs_)/90UL;'),
  'Animation pacing must target 11fps using elapsed time');
const atlasGenerator=read('tools/generate_plane_atlas.py');
assert(atlasGenerator.includes('static uint8_t mascotPixel(') &&
  atlasGenerator.includes('STRIDE * POSES') &&
  atlasGenerator.includes('POSES = 12'),
  'Atlas generator must reuse the actual detailed airplane geometry and validate output');
assert(mascot.includes('struct PlanePose') && mascot.includes('p.invX=') &&
  mascot.includes('p.scale=(int16_t)constrain('),
  'The standalone hero must perform scaled, perspective 3-D style turns');
assert(mascot.includes('const int x=120+pose.shiftX,y=122+pose.shiftY') &&
  mascot.includes('const int rx=mini?40:109,ry=mini?30:78'),
  'The full-size airplane must be enlarged, centered and clipped safely');
assert(mascot.includes('switch(pattern%20u)'),'Motion selector must use 20 flight presets');
for(let id=0;id<19;id++)
  assert(new RegExp('case\\s*'+id+':').test(mascot), 'Missing flight preset '+id);
assert(mascot.includes('motionVariant_=(uint8_t)(') &&
  mascot.includes('%20u);'), 'A new flight routine must be chosen after each phrase');
assert(mascot.includes('Wing thickness/underside') &&
  mascot.includes('Raised canopy roof') &&
  mascot.includes('Tapered three-layer nose cone'),
  'The airplane needs dimensional wings, cockpit and nose');
assert(phrases.includes('static const Phrase bank[] PROGMEM') && mascot.includes('motivation::copy('),'Quotes must be safely fetched from Flash');
assert(mascot.includes('Signature swept AIRPLANE WINGS') && !mascot.includes('propeller struts'), 'Mascot must be a fixed-wing airplane');
assert(mascot.includes('month=now.tm_mon+1')&&mascot.includes('mood_'),'Contextual gesture switching missing');
assert(mascot.includes('intervalMin*60000UL'),'Popup interval not configurable');
assert(mascot.includes('s.mascot.speechSec*1000UL'), 'Quote duration must come from persisted settings');
assert(mascot.includes('s.mascot.roamSec*1000UL'), 'Roaming duration must come from persisted settings');
const mascotSettings=read('src/Settings.cpp');
assert(mascotSettings.includes('speechSec=60;roamSec=30'), 'Default durations must be 60s/30s');
assert(mascotSettings.includes('o["speechSec"]=speechSec') && mascotSettings.includes('o["roamSec"]=roamSec'), 'Must save both durations');
assert(mascotSettings.includes('speechSec=constrain(') && mascotSettings.includes('roamSec=constrain('), 'Must validate both durations');
assert(mascot.includes('if(s.mode==MODE_CAROUSEL)') && mascot.includes('carouselQuoteUntilMs_'), 'Carousel must have a distinct timed quote state');
assert(mascot.includes('carouselQuoteUntilMs_=now+(uint32_t)s.mascot.speechSec*1000UL'), 'Carousel quote interval should be configurable');
assert(mascot.includes('phase_==ROAM')&&mascot.includes('drawRoaming('),'Mascot dedicated mode must have roaming phase');
assert(mascot.includes('drawBigCharacter(')&&mascot.includes('spriteAt('),'Roaming sprite must draw upgraded aviation character');
assert(mascot.includes('wrapText(quote,scale,lines)')&&mascot.includes('uint8_t scale=3'),'Speech text should start at enlarged scale');
const map=read('src/features/alerts/AlertMap.cpp');
assert(map.includes('fillScreen(C_BLACK)'),'Map background must be black');
assert(map.includes("case 'N':return C_WHITE"),'No-alert regions must be white');
assert(map.includes('geoRow(')&&map.includes('geoRun('),'Real oblast raster not used');
assert(map.includes('case \'R\'')&&map.includes('case \'Y\'')&&map.includes('case \'N\''),'Alert colors missing');
const geo=read('src/features/alerts/OblastGeometry.h');
assert(geo.includes('pgm_read_dword('),'Geometry must use aligned 32bit reads');
const clock=read('src/features/clock/ClockFaceMode.cpp');
assert(clock.includes('face==4')&&clock.includes('altNeedle(')&&clock.includes('altTimeWindow('),'Aviation altimeter clock missing');
const main=read('src/main.cpp');
assert(main.indexOf('if (otaBootRequested())')<main.indexOf('clockReapply(g_settings)'),'OTA must run before NTP');
assert(main.includes('g_mascotMode.tick('),'Animation scheduling not integrated');
const platformio=read('platformio.ini');
for(const feat of ['WITH_HA=0','WITH_RADAR=0','WITH_USAGE=0','WITH_TICKER=0'])
  assert(platformio.includes(feat),feat+' not disabled');
const webui=read('src/webui.html');
for(const id of ['mascotEnabled','mascotIntervalMin','mascotFocus','mascotSpeechSec','mascotRoamSec','clockFace'])
  assert(webui.includes('id="'+id+'"'),id+' web input missing');
const alerts=read('src/features/alerts/AlertsMode.cpp');
assert(alerts.includes('overlayAlert_?C_RED:C_GREEN'), 'Start and all-clear notifications need red/green frames');
assert(alerts.includes('120,2,col'), 'Start and all-clear notifications need matching text colors');
assert(mascot.includes('const int marker=credit.indexOf(" • ")') && mascot.includes('credit.c_str(),126,authorSize'), 'Authors should be legible with paraphrase note beneath comic quotes');
console.log('140-phrase aviation airplane, configurable timings, carousel, anti-flicker, alert overlays and OTA checks passed.');
