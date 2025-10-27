#ifndef _CANDEFS
#define _CANDEFS

#include "inttypes.h"

typedef struct __attribute__((packed)) {
  uint8_t throttle_position;
  uint8_t rear_brake_pressure;
  uint8_t front_brake_pressure;
  uint8_t unused[5];
} AnalogData;
_Static_assert(sizeof(AnalogData) == 8, "AnalogData defined incorrectly");

typedef struct __attribute__((packed)) {
  int16_t acc_x;
  int16_t acc_y;
  int16_t acc_z;
  int16_t gyro_x;
} IMUData1;
_Static_assert(sizeof(IMUData1) == 8, "IMUData1 defined incorrectly");

typedef struct __attribute__((packed)) {
  int16_t gyro_y;
  int16_t gyro_z;
  uint16_t baro;
  uint8_t unused[2];
} IMUData2;
_Static_assert(sizeof(IMUData2) == 8, "IMUData2 defined incorrectly");

typedef struct {
  const uint32_t id;
  union {
    AnalogData analog;
    IMUData1 imu1;
    IMUData2 imu2;
    uint8_t def[8];
  } data;
} CanMsg;

#define INIT_AnalogMsg(X) CanMsg X = {.id = 0x21, .data = {.def = {0}}}
#define INIT_IMUMsg1(X) CanMsg X = {.id = 0x22, .data = {.def = {0}}}
#define INIT_IMUMsg2(X) CanMsg X = {.id = 0x23, .data = {.def = {0}}}

// typedef struct {
//   const uint32_t id;
//   AnalogData data;
// } AnalogMsg;
// typedef struct {
//   const uint32_t id;
//   IMUData1 data;
// } IMUMsg1;
// typedef struct {
//   const uint32_t id;
//   IMUData2 data;
// } IMUMsg2;

#endif // !_CANDEFS
