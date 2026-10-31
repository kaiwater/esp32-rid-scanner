#include "rid_parse.h"

static void copyId(char *dst, size_t dstSize, const char *src)
{
  memset(dst, 0, dstSize);
  if (!src) return;
  strncpy(dst, src, dstSize - 1);
}

bool ridFillFromUas(ODID_UAS_Data *uas, RidDrone *out)
{
  if (!uas || !out) return false;

  out->hasBasicId    = false;
  out->hasOperatorId = false;
  out->hasLocation   = false;
  out->hasSystem     = false;

  bool useful = false;

  if (uas->BasicIDValid[0]) {
    out->hasBasicId = true;
    copyId(out->uasId, sizeof(out->uasId), uas->BasicID[0].UASID);
    useful = true;
  }

  if (uas->OperatorIDValid) {
    out->hasOperatorId = true;
    copyId(out->opId, sizeof(out->opId), uas->OperatorID.OperatorId);
    useful = true;
  }

  if (uas->LocationValid) {
    out->hasLocation = true;
    out->lat      = uas->Location.Latitude;
    out->lon      = uas->Location.Longitude;
    out->altGeo   = uas->Location.AltitudeGeo;
    out->height   = uas->Location.Height;
    out->speedH   = uas->Location.SpeedHorizontal;
    out->heading  = uas->Location.Direction;
    useful = true;
  }

  if (uas->SystemValid) {
    double opLat = uas->System.OperatorLatitude;
    double opLon = uas->System.OperatorLongitude;
    if (!(opLat == 0.0 && opLon == 0.0)) {
      out->hasSystem = true;
      out->opLat = opLat;
      out->opLon = opLon;
      useful = true;
    }
  }

  return useful;
}

bool ridParseOdid(const uint8_t *msg, size_t len, ODID_UAS_Data *uas, RidDrone *out)
{
  if (!msg || len < 1 || !uas || !out) return false;

  odid_initUasData(uas);

  if ((msg[0] & 0xF0) == (ODID_MESSAGETYPE_PACKED << 4)) {
    if (odid_message_process_pack(uas, (uint8_t *)msg, len) < 0) return false;
  } else {
    ODID_messagetype_t type = decodeOpenDroneID(uas, (uint8_t *)msg);
    if (type == ODID_MESSAGETYPE_INVALID) return false;
  }

  return ridFillFromUas(uas, out);
}
