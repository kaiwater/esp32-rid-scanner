#pragma once

#include "rid_types.h"

void ridStoreInit();
void ridStoreUpsert(const RidDrone &incoming);
void ridStoreExpire(uint32_t now);
void ridStoreClear();

// Copies currently active drones, newest heard first. Returns the count.
int ridStoreSnapshot(RidDrone *out, int maxCount, uint32_t now);
int ridStoreCount(uint32_t now);
