#!/usr/bin/env node
// Firmware anti-flicker regression checks: keep expensive redraws out of
// recurring service paths. These are source-level checks, not hardware tests.
const fs=require('node:fs');
const assert=require('node:assert/strict');
const clock=fs.readFileSync('src/features/clock/ClockFaceMode.cpp','utf8');
const alerts=fs.readFileSync('src/features/alerts/AlertsMode.cpp','utf8');
const map=fs.readFileSync('src/features/alerts/AlertMap.cpp','utf8');
const clockPartial=clock.split('void ClockFaceMode::updateTime(')[1].split('void ClockFaceMode::service(')[0];
const clockService=clock.split('void ClockFaceMode::service(')[1];
assert(!clockPartial.includes('fillScreen('), 'One-second clock update must never clear the full screen');
assert(clockPartial.includes('if(minuteChanged)'), 'Digital minutes should refresh only on a minute change');
assert(clockPartial.includes('hands(gfx,lastHour_'), 'Analog clock should erase only the old hands');
assert(clockService.includes('t.tm_sec!=lastSecond_'), 'Second hand should still be updated once per second');
assert(alerts.includes('if(overlayDrawnAt_)return;'), 'Static overlay must paint once per event');
assert(!alerts.includes('millis()-lastRenderMs_'), 'Map must not repaint every N seconds');
assert(alerts.includes('memcmp(oldStatuses,oblasts_'), 'Map redraw must depend on real API changes');
assert(alerts.includes('paintedFresh_!=fresh(s)'), 'Stale data must still trigger a grey map');
assert(map.includes('nearestRegion('), 'Map must render 27 color-coded territories');
assert(map.includes('alertMapDraw('), 'Map renderer missing');
assert(map.includes('if(fullFrame)'), 'Map must initialize frame only on entry');
assert(map.includes('gfx->drawFastHLine(runStart,y,x-runStart,color)'), 'Map uses row runs, not per-pixel SPI fills');
console.log('Display anti-flicker and map checks passed.');
