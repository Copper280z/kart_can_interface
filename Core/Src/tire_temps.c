#include "tire_temps.h"
#include "can.h"
#include "can_defs.h"
#include "dbc_assert.h"
#include "main.h"
#include "sst.h"
#include "stm32h743xx.h"
#include "stm32h7xx_hal_def.h"
#include "stm32h7xx_hal_i2c.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

DBC_MODULE_NAME("Tire Sensors")

static INIT_IMUMsg1(msg);
uint8_t raw_pix_buffer[128] = {0};
uint16_t pixel_buffer[64] = {0};
uint8_t sensor_idx;

enum Tire_Signals {
  START_MEASURE_SIG,
  /* ... */
  TIRE_MAX_SIG /* the last signal */
};
typedef struct {
  SST_Task super; /* inherit SST_Task  */
  SST_TimeEvt
      handler_evt; // tasks can have multiple time events, for different actions
  I2C_HandleTypeDef *hi2c;
  uint8_t num_sensors;
  uint8_t sensor_addr[4];
  uint8_t sensor_type[4];
} Tire_Task;

static Tire_Task tire_inst;
SST_Task *const AO_tires = &tire_inst.super; // task might be defined in

static void Tire_ctor(Tire_Task *const me);
static void Tire_init(Tire_Task *const me, SST_Evt const *const ie);
static void Tire_dispatch(Tire_Task *const me, SST_Evt const *const e);
void start_measurement(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t type);
void DCMI_IRQHandler();

void tire_temps_task_instantiate(void (*error_callback)(),
                                 I2C_HandleTypeDef *hi2c, uint8_t num_sensors,
                                 uint8_t sensor_addr[4],
                                 uint8_t sensor_type[4]) {
  assert(num_sensors < 5);
  tire_inst.num_sensors = num_sensors;
  tire_inst.hi2c = hi2c;
  memcpy(&tire_inst.sensor_addr, sensor_addr, num_sensors);
  memcpy(&tire_inst.sensor_type, sensor_type, num_sensors);
  Tire_ctor(&tire_inst);
  SST_Task_setIRQ(AO_tires, DCMI_IRQn);
}
static void Tire_ctor(Tire_Task *const me) {
  SST_Task_ctor(&me->super, (SST_Handler)&Tire_init,
                (SST_Handler)&Tire_dispatch);
  SST_TimeEvt_ctor(&me->handler_evt, START_MEASURE_SIG, &me->super);
}
static void Tire_init(Tire_Task *const me, SST_Evt const *const ie) {
  sensor_idx = 0;
  SST_TimeEvt_arm(&me->handler_evt, 100U, 100U);
}
static void Tire_dispatch(Tire_Task *const me, SST_Evt const *const e) {
  switch (e->sig) {
  case START_MEASURE_SIG: {
    sensor_idx = 0;
    start_measurement(tire_inst.hi2c, tire_inst.sensor_addr[0],
                      tire_inst.sensor_type[0]);
    break;
  }
  default:
    DBC_ERROR(200);
  }
}

void start_measurement(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t type) {
  switch (type) {
  case 0: {
    // AMG8833
    printf("Start Tire Measurement: addr: 0x%x\r\n", addr);
    uint8_t buffer[1] = {0x80};
    HAL_StatusTypeDef status =
        HAL_I2C_Master_Transmit(hi2c, addr << 1, buffer, 1, 1000);
    if (status != HAL_OK) {
      printf("Failed to start measure tires with error: %d\r\n", status);
    }
    status = HAL_I2C_Master_Receive_IT(hi2c, addr << 1, raw_pix_buffer, 128);
    if (status != HAL_OK) {
      printf("Failed to recv measure tires with error: %d\r\n", status);
    }
    break;
  }
  case 1: {
    // MLX90640 or similar
    break;
  }
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  for (int i = 0; i < 64; i++) {
    uint8_t pos = i << 1;
    uint16_t recast = ((uint16_t)raw_pix_buffer[pos + 1] << 8) |
                      ((uint16_t)raw_pix_buffer[pos]);

    pixel_buffer[i] = recast / 4; // TODO: div by 4 is the conversion factor,
                                  // but it truncates decimals and isn't signed
  }
  // can_tx_msg(&msg);
  printf("Tire %d pixels: %d, %d, %d, %d\r\n", sensor_idx, pixel_buffer[0],
         pixel_buffer[1], pixel_buffer[2], pixel_buffer[3]);

  if (sensor_idx < (tire_inst.num_sensors - 1)) {
    sensor_idx += 1;
    start_measurement(hi2c, tire_inst.sensor_addr[sensor_idx],
                      tire_inst.sensor_type[sensor_idx]);
  } else {
    sensor_idx = 0;
  }
}
void DCMI_IRQHandler() {
  SST_Task_activate(AO_tires); //
}
