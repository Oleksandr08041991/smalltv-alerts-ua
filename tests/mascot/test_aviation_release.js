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
const head=mascot.split('void MascotMode::drawCharacter(')[0];
const character=mascot.split('void MascotMode::drawSmallCharacter(')[1].split('void MascotMode::drawBigCharacter(')[0];
assert(!character.includes('fillScreen('),'Animation must not blank the screen');
assert(character.includes('mouthOpen')&&character.includes('blink')&&character.includes('wing'),'Missing animated facial/wing parts');
assert(mascot.includes('month=now.tm_mon+1')&&mascot.includes('mood_'),'Contextual gesture switching missing');
assert(mascot.includes('intervalMin*60000UL'),'Popup interval not configurable');
assert(mascot.includes('kRoamMs=60000UL'),'Tamagotchi must roam for one minute between phrases');
assert(mascot.includes('kSpeechMs=15000UL'),'Speech duration must be 15 seconds');
assert(mascot.includes('phase_==ROAM')&&mascot.includes('drawRoaming('),'Mascot dedicated mode must have roaming phase');
assert(mascot.includes('drawBigCharacter(')&&mascot.includes('prevX_-70'),'Roaming sprite must be large and partially repainted');
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
for(const id of ['mascotEnabled','mascotIntervalMin','mascotFocus','clockFace'])
  assert(webui.includes('id="'+id+'"'),id+' web input missing');
console.log('100-phrase aviation mascot, map, altimeter, OTA and web UI regression checks passed.');
