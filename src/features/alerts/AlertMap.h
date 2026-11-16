// Compact 240x240 regional choropleth for SmallTV Ultra.
// The tessellation is a schematic display of the 27 API units, NOT a map
// of exact legal administrative boundaries.
#pragma once
#include <Arduino.h>
class Arduino_GFX;
int alertMapRegionIndex(uint16_t uid);
void alertMapDraw(Arduino_GFX* gfx, const char regionStatuses[28],
                  bool usable, uint16_t selectedUid, bool fullFrame);
