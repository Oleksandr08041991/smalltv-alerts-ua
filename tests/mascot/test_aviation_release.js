#!/usr/bin/env node
// Source checks for SmallTV ESP8266 mascot, map, altimeter and OTA workflows.
// These do not replace physical-device testing.
const fs=require('node:fs'),assert=require('node:assert/strict');
const read=f=>fs.readFileSync(f,'utf8');
const phrases=read('src/features/mascot/MotivationPhrases.h');
const count=(phrases.match(/^  \{"/gm)||[]).length;
assert.equal(count,100,'Must bundle exactly 100 offline Ukrainian phrases');
assert(phrases.includes('Леся Українка')&&phrases.includes('Іван Франко'),'Literary authors missing');
assert(phrases.includes('Франклін • переказ'),'Franklin paraphrases must be identified as such');
assert(phrases.includes('Оригінальна фраза'),'Original text must not get literary attribution');
const mascot=read('src/features/mascot/MascotMode.cpp');
const renderer=mascot.split('static uint8_t mascotPixel(')[1].split('static uint16_t mascotColor(')[0];
assert(renderer.includes('blink') && renderer.includes('flap') && renderer.includes('grin'), 'Animated eyes, wings and smile missing');
const delta=mascot.split('static void drawSpriteDelta(')[1].split('} // namespace')[0];
assert(!delta.includes('fillScreen(') && !delta.includes('fillRect('), 'Walking must update only changed pixels without erasing rectangles');
assert(mascot.includes('drawSpriteDelta('), 'Sprite delta-rendering missing');
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
assert(mascot.includes('const uint8_t authorSize=') && mascot.includes('p.author,136,authorSize'), 'Authors should be legible beneath comic quotes');
console.log('100-phrase aviation mascot, configurable timings, carousel, anti-flicker, alert overlays and OTA checks passed.');
