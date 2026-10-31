#pragma once

#include "rid_types.h"
#include "rid_wifi.h"

void uiInit();
void uiDrawBoot();
void uiDrawList(const RidDrone *list, int count, int selected, uint32_t now);
void uiDrawDetail(const RidDrone &d, int index, int total, uint32_t now, int page);
void uiDrawRfScan(const RidWifiRfCh *stats, int currentIdx, int currentChannel, uint32_t now);
void uiDrawFatal(const char *line1, const char *line2);
