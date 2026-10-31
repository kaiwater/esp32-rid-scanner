#pragma once

#include "rid_types.h"

// Fills `out` from already-decoded UAS data. `out` must already contain
// mac/source/rssi/lastSeen. Returns true if at least one useful field exists.
bool ridFillFromUas(ODID_UAS_Data *uas, RidDrone *out);

// Decodes one OpenDroneID message or message pack (without the BLE/WiFi
// transport wrapper) and fills `out`.
bool ridParseOdid(const uint8_t *msg, size_t len, ODID_UAS_Data *uas, RidDrone *out);
